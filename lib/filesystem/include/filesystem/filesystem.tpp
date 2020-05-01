namespace
{
    template <typename Iterator>
    auto do_metadata_op(rxComm& _comm,
                        const path& _path,
                        Iterator _first,
                        Iterator _last,
                        std::string_view _op) -> void
    {
        using detail::make_error_code;

        std::string_view entity_type;

        if (const auto s = status(_comm, _path); is_data_object(s)) {
            entity_type = "data_object";
        }
        else if (is_collection(s)) {
            entity_type = "collection";
        }
        else {
            throw filesystem_error{"cannot apply metadata operations: unknown object type", _path,
                                   make_error_code(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION)};
        }

        using json = nlohmann::json;

        std::vector<json> operations;

        std::for_each(_first, _last, [_op, &operations](const metadata& md) {
            operations.push_back({
                {"operation", _op},
                {"attribute", md.attribute},
                {"value", md.value},
                {"units", md.units}
            });
        });

        const auto json_input = json{
            {"entity_name", _path.c_str()},
            {"entity_type", entity_type},
            {"operations", operations}
        }.dump();

        char* json_error_string{};

        if (const auto ec = rx_atomic_apply_metadata_operations(&_comm, json_input.data(), &json_error_string); ec) {
            throw filesystem_error{"cannot apply metadata operations", _path, make_error_code(ec)};
        }
    }
} // anonymous namespace

template <typename Iterator>
auto add_metadata(rxComm& _comm, const path& _path, Iterator _first, Iterator _last) -> void
{
    do_metadata_op(_comm, _path, _first, _last, "add");
}

template <typename Container, typename, typename>
auto add_metadata(rxComm& _comm, const path& _path, const Container& _container) -> void
{
    add_metadata(_comm, _path, std::begin(_container), std::end(_container));
}

template <typename Iterator>
auto remove_metadata(rxComm& _comm, const path& _path, Iterator _first, Iterator _last) -> void
{
    do_metadata_op(_comm, _path, _first, _last, "remove");
}

template <typename Container, typename, typename>
auto remove_metadata(rxComm& _comm, const path& _path, const Container& _container) -> void
{
    remove_metadata(_comm, _path, std::begin(_container), std::end(_container));
}
