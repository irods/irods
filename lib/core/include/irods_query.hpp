#ifndef IRODS_QUERY_HPP
#define IRODS_QUERY_HPP

#include "specificQuery.h"

#ifdef IRODS_QUERY_ENABLE_SERVER_SIDE_API
    #include "rsGenQuery.hpp"
    #include "rsSpecificQuery.hpp"
#else
    #include "genQuery.h"
#endif // IRODS_QUERY_ENABLE_SERVER_SIDE_API

#include "irods_log.hpp"
#include "rcMisc.h"

#include <algorithm>
#include <string>
#include <vector>

char *getCondFromString( char * t );

namespace irods {

    template <typename connection_type>
    class query {
    public:
        using value_type = std::vector<std::string>;

        enum query_type {
            GENERAL = 0,
            SPECIFIC = 1
        };

        static query_type convert_string_to_query_type(
                const std::string& _str) {
            // default option
            if(_str.empty()) {
                return GENERAL;
            }

            const std::string GEN_STR{"general"};
            const std::string SPEC_STR{"specific"};

            std::string lowered{_str};
            std::transform(
                lowered.begin(),
                lowered.end(),
                lowered.begin(),
                ::tolower);

            if(GEN_STR == lowered) {
                return GENERAL;
            }
            else if(SPEC_STR == lowered) {
                return SPECIFIC;
            }
            else {
                THROW(
                    SYS_INVALID_INPUT_PARAM,
                    _str + " - is not a query type");
            }
        } // convert_string_to_query_type

        class query_impl_base
        {
        public:
            query_impl_base(connection_type*   _comm,
                            const uint32_t     _query_limit,
                            const uint32_t     _row_offset,
                            const std::string& _query_string)
                : comm_{_comm}
                , query_limit_{_query_limit}
                , row_offset_{_row_offset}
                , query_string_{_query_string}
                , gen_output_{}
            {
            }

            virtual ~query_impl_base() {
                freeGenQueryOut(&this->gen_output_);
            }

            size_t size() {
                if(!gen_output_) {
                    return 0;
                }
                return gen_output_->rowCnt;
            }

            int cont_idx() {
                return gen_output_->continueInx;
            }

            int row_cnt() {
                return gen_output_->rowCnt;
            }

            std::string query_string() {
                return query_string_;
            }

            bool query_limit_exceeded(const uint32_t _count) {
                return query_limit_ && _count >= query_limit_;
            }

            bool page_in_flight(const int row_idx_) {
                return (row_idx_ < row_cnt());
            }

            bool query_complete() {
                // finished page, and out of pages
                return cont_idx() <= 0;
            }

            value_type capture_results(int _row_idx) {
                value_type res;
                for(int attr_idx = 0; attr_idx < gen_output_->attriCnt; ++attr_idx) {
                    uint32_t offset = gen_output_->sqlResult[attr_idx].len * _row_idx;
                    std::string str{&gen_output_->sqlResult[attr_idx].value[offset]};
                    res.push_back(str);
                }
                return res;
            }

            bool results_valid() {
                if(gen_output_) {
                    return (gen_output_->rowCnt > 0);
                }
                else {
                    return false;
                }
            }

            virtual int fetch_page() = 0;

            virtual void reset_for_page_boundary() = 0;

        protected:
            connection_type* comm_;
            const uint32_t query_limit_;
            const uint32_t row_offset_;
            const std::string query_string_;
            genQueryOut_t* gen_output_;
        }; // class query_impl_base

        class gen_query_impl : public query_impl_base
        {
        public:
            gen_query_impl(connection_type*   _comm,
                           int                _query_limit,
                           int                _row_offset,
                           const std::string& _query_string,
                           const std::string& _zone_hint)
                : query_impl_base(_comm, _query_limit, _row_offset, _query_string)
            {
                memset(&gen_input_, 0, sizeof(gen_input_));
                gen_input_.maxRows = MAX_SQL_ROWS;
                gen_input_.rowOffset = _row_offset;

                if (!_zone_hint.empty()) {
                    addKeyVal(&gen_input_.condInput, ZONE_KW, _zone_hint.c_str());
                }

                const int fill_err = fillGenQueryInpFromStrCond(
                                         const_cast<char*>(_query_string.c_str()),
                                         &gen_input_);
                if(fill_err < 0) {
                    THROW(
                        fill_err,
                        boost::format("query fill failed for [%s]") %
                        _query_string);
                }
            } // ctor

            virtual ~gen_query_impl() {
                if(this->gen_output_ && this->gen_output_->continueInx) {
                    // Close statements for this query
                    gen_input_.continueInx = this->gen_output_->continueInx;
                    freeGenQueryOut(&this->gen_output_);
                    gen_input_.maxRows = 0;
                    auto err = gen_query_fcn(
                                   this->comm_,
                                   &gen_input_,
                                   &this->gen_output_);
                    if (CAT_NO_ROWS_FOUND != err && err < 0) {
                        irods::log(ERROR(err, (boost::format(
                                    "[%s] - Failed to close statement with continueInx [%d]") %
                                    __FUNCTION__ % gen_input_.continueInx).str()));
                    }
                }

                clearGenQueryInp(&gen_input_);
            }

            void reset_for_page_boundary() override {
                if(this->gen_output_) {
                    gen_input_.continueInx = this->gen_output_->continueInx;
                    freeGenQueryOut(&this->gen_output_);
                }
            }

            int fetch_page() override {
                return gen_query_fcn(
                           this->comm_,
                           &gen_input_,
                           &this->gen_output_);
            } // fetch_page

        private:
            genQueryInp_t gen_input_;
#ifdef IRODS_QUERY_ENABLE_SERVER_SIDE_API
            const std::function<
                int(connection_type*,
                    genQueryInp_t*,
                    genQueryOut_t**)>
                        gen_query_fcn{rsGenQuery};
#else
            const std::function<
                int(connection_type*,
                    genQueryInp_t*,
                    genQueryOut_t**)>
                        gen_query_fcn{rcGenQuery};
#endif // IRODS_QUERY_ENABLE_SERVER_SIDE_API
        }; // class gen_query_impl

        class spec_query_impl : public query_impl_base
        {
        public:
            spec_query_impl(connection_type*                _comm,
                            int                             _query_limit,
                            int                             _row_offset,
                            const std::string&              _query_string,
                            const std::string&              _zone_hint,
                            const std::vector<std::string>* _args)
                : query_impl_base(_comm, _query_limit, _row_offset, _query_string)
            {
                memset(&spec_input_, 0, sizeof(spec_input_));
                spec_input_.maxRows = MAX_SQL_ROWS;
                spec_input_.sql = const_cast<char*>(_query_string.c_str());

                if (!_zone_hint.empty()) {
                    addKeyVal(&spec_input_.condInput, ZONE_KW, _zone_hint.c_str());
                }

                if (_args) {
                    for (decltype(_args->size()) i = 0; i < _args->size(); ++i) {
                        spec_input_.args[i] = const_cast<char*>((*_args)[i].data());
                    }
                }
            } // ctor

            virtual ~spec_query_impl() {
                if(this->gen_output_ && this->gen_output_->continueInx) {
                    // Close statement for this query
                    spec_input_.continueInx = this->gen_output_->continueInx;
                    freeGenQueryOut(&this->gen_output_);
                    spec_input_.maxRows = 0;
                    auto err = spec_query_fcn(
                                   this->comm_,
                                   &spec_input_,
                                   &this->gen_output_);
                    if (CAT_NO_ROWS_FOUND != err && err < 0) {
                        irods::log(ERROR(
                                    err, (boost::format(
                                    "[%s] - Failed to close statement with continueInx [%d]") %
                                    __FUNCTION__ % spec_input_.continueInx).str()));
                    }
                }

                clearKeyVal(&spec_input_.condInput);
            }

            void reset_for_page_boundary() override {
                if(this->gen_output_) {
                    spec_input_.continueInx = this->gen_output_->continueInx;
                    freeGenQueryOut(&this->gen_output_);
                }
            }

            int fetch_page() override {
                return spec_query_fcn(
                           this->comm_,
                           &spec_input_,
                           &this->gen_output_);
            } // fetch_page

        private:
            specificQueryInp_t spec_input_;
#ifdef IRODS_QUERY_ENABLE_SERVER_SIDE_API
            const std::function<
                int(connection_type*,
                    specificQueryInp_t*,
                    genQueryOut_t**)>
                        spec_query_fcn{rsSpecificQuery};
#else
            const std::function<
                int(connection_type*,
                    specificQueryInp_t*,
                    genQueryOut_t**)>
                        spec_query_fcn{rcSpecificQuery};
#endif // IRODS_QUERY_ENABLE_SERVER_SIDE_API
        }; // class spec_query_impl

        class iterator {
            const std::string query_string_;
            uint32_t row_idx_;
            uint32_t total_rows_processed_;
            genQueryInp_t* gen_input_;
            bool end_iteration_state_;

            std::shared_ptr<query_impl_base> query_impl_;

            public:
            using value_type        = value_type;
            using pointer           = const value_type*;
            using reference         = value_type;
            using difference_type   = value_type;
            using iterator_category = std::forward_iterator_tag;

            explicit iterator() :
                query_string_{},
                row_idx_{},
                total_rows_processed_{},
                gen_input_{},
                end_iteration_state_{true},
                query_impl_{} {
            }

            explicit iterator(std::shared_ptr<query_impl_base> _qimp) :
                query_string_{},
                row_idx_{},
                total_rows_processed_{},
                gen_input_{},
                end_iteration_state_{false},
                query_impl_(_qimp) {
            }

            explicit iterator(
                const std::string&       _query_string,
                genQueryInp_t*           _gen_input) :
                query_string_{_query_string},
                row_idx_{},
                total_rows_processed_{},
                gen_input_{_gen_input},
                end_iteration_state_{false},
                query_impl_{} {
            } // ctor

            iterator operator++() {
                advance_query();
                return *this;
            }

            iterator operator++(int) {
                iterator ret = *this;
                ++(*this);
                return ret;
            }

            bool operator==(const iterator& _rhs) const {
                if(end_iteration_state_ && _rhs.end_iteration_state_) {
                    return true;
                }

                return (query_impl_->query_string() == _rhs.query_string_);
            }

            bool operator!=(const iterator& _rhs) const {
                return !(*this == _rhs);
            }

            value_type operator*() {
                return capture_results();
            }

            void reset_for_page_boundary() {
                row_idx_ = 0;
                query_impl_->reset_for_page_boundary();
            }

            void advance_query() {
                total_rows_processed_++;
                if(query_impl_->query_limit_exceeded(total_rows_processed_)) {
                    end_iteration_state_ = true;
                    return;
                }

                row_idx_++;
                if(query_impl_->page_in_flight(row_idx_)) {
                    return;
                }

                if(query_impl_->query_complete()) {
                    end_iteration_state_ = true;
                    return;
                }

                reset_for_page_boundary();
                const int query_err = query_impl_->fetch_page();
                if(query_err < 0) {
                    if(CAT_NO_ROWS_FOUND != query_err) {
                        THROW(
                            query_err,
                            boost::format("gen query failed for [%s] on idx %d") %
                            query_string_ %
                            gen_input_->continueInx);
                    }

                   end_iteration_state_ = true;

                } // if

            } // advance_query 

            value_type capture_results() {
                return query_impl_->capture_results(row_idx_);
            }
        }; // class iterator

        query(connection_type*                _comm,
              const std::string&              _query_string,
              const std::vector<std::string>* _specific_query_args,
              const std::string&              _zone_hint,
              uintmax_t                       _query_limit,
              uintmax_t                       _row_offset,
              query_type                      _query_type)
            : iter_{}
            , query_impl_{}
        {
            if(_query_type == GENERAL) {
                query_impl_ = std::make_shared<gen_query_impl>(
                                  _comm,
                                  _query_limit,
                                  _row_offset,
                                  _query_string,
                                  _zone_hint);
            }
            else if(_query_type == SPECIFIC) {
                query_impl_ = std::make_shared<spec_query_impl>(
                                  _comm,
                                  _query_limit,
                                  _row_offset,
                                  _query_string,
                                  _zone_hint,
                                  _specific_query_args);
            }

            const int fetch_err = query_impl_->fetch_page();
            if(fetch_err < 0) {
                if(CAT_NO_ROWS_FOUND == fetch_err) {
                    iter_ = std::make_unique<iterator>();
                }
                else {
                    THROW(
                        fetch_err,
                        boost::format("query failed for [%s] type [%d]") %
                        _query_string %
                        _query_type);
                }
            }

            if(query_impl_->results_valid()) {
                iter_ = std::make_unique<iterator>(query_impl_);
            }
            else {
                iter_ = std::make_unique<iterator>();
            }
        } // ctor

        query(connection_type*   _comm,
              const std::string& _query_string,
              uintmax_t          _query_limit = 0,
              uintmax_t          _row_offset  = 0,
              query_type         _query_type  = GENERAL)
            : query{_comm, _query_string, nullptr, {}, _query_limit, _row_offset, _query_type}
        {
        } // ctor

        query(query&&) = default;
        query& operator=(query&&) = default;

        ~query() {}

        iterator   begin() { return *iter_; }

        iterator   end()   { return iterator(); }

        value_type front() { return (*(*iter_)); }

        value_type front() const { return (*(*iter_)); }

        size_t size()  { return query_impl_->size(); }

        size_t size() const { return query_impl_->size(); }

        size_t empty() { return 0 == query_impl_->size(); }

        size_t empty() const { return 0 == query_impl_->size(); }

    private:
        std::unique_ptr<iterator>        iter_;
        std::shared_ptr<query_impl_base> query_impl_;
    }; // class query
} // namespace irods

#endif // IRODS_QUERY_HPP

