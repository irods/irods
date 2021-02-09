#include "catch.hpp"

#include "irods_error_enum_matcher.hpp"
#include "rodsError.h"

#include <cstring>
#include "fmt/format.h"

TEST_CASE("test_ErrorStack", "[ErrorStack][add][free][print]")
{
    constexpr int message_count = 9;

    ErrorStack stack{};

    for (int i = 0; i < message_count; ++i) {
        const std::string msg = fmt::format("message {}", i);
        REQUIRE(0 == addRErrorMsg(&stack, i, msg.data()));

        const auto& error_message = stack.errMsg[i];
        REQUIRE(error_message);
    }

    REQUIRE(stack.errMsg);
    REQUIRE(message_count == stack.len);

    SECTION("replErrorStack")
    {
        ErrorStack* stack2 = static_cast<ErrorStack*>(std::malloc(sizeof(ErrorStack)));
        std::memset(stack2, 0, sizeof(ErrorStack));

        REQUIRE(0 == replErrorStack(&stack, stack2));
        REQUIRE(stack2->len == stack.len);
        REQUIRE(stack2->errMsg);

        for (int i = 0; i < message_count; ++i) {
            const auto& msg2 = stack2->errMsg[i];
            REQUIRE(msg2);

            const auto& msg = stack.errMsg[i];

            CHECK(msg2 != msg);

            CHECK(msg2->status == msg->status);
            CHECK(std::string_view{msg2->msg} == msg->msg);
        }

        CHECK(0 == freeRError(stack2));
    }

    SECTION("pop message")
    {
        constexpr int new_message_status_1 = message_count * 2;
        constexpr int new_message_status_2 = message_count * 2 + 1;
        const std::string new_message_1 = "pop me";
        const std::string new_message_2 = "pop me too";

        REQUIRE(0 == addRErrorMsg(&stack, new_message_status_1, new_message_1.data()));
        CHECK(message_count + 1 == stack.len);

        REQUIRE(0 == addRErrorMsg(&stack, new_message_status_2, new_message_2.data()));
        CHECK(message_count + 2 == stack.len);

        CHECK(new_message_2 == irods::pop_error_message(stack));
        CHECK(message_count + 1 == stack.len);

        CHECK(new_message_1 == irods::pop_error_message(stack));
        CHECK(message_count == stack.len);
    }

    SECTION("exceed max error messages")
    {
        constexpr int last_index = MAX_ERROR_MESSAGES - 1;

        for (int i = message_count; i < MAX_ERROR_MESSAGES; ++i) {
            const std::string msg = fmt::format("message {}", i);
            REQUIRE(0 == addRErrorMsg(&stack, i, msg.data()));

            const auto& error_message = stack.errMsg[i];
            REQUIRE(error_message);
        }

        REQUIRE(MAX_ERROR_MESSAGES == stack.len);

        CHECK(0 == addRErrorMsg(&stack, last_index, "nope"));

        CHECK(MAX_ERROR_MESSAGES == stack.len);
        CHECK(last_index == stack.errMsg[stack.len - 1]->status);
        CHECK(std::string{fmt::format("message {}", last_index)} == stack.errMsg[stack.len - 1]->msg);
    }

    REQUIRE(0 == freeRErrorContent(&stack));
}

TEST_CASE("invalid inputs", "[ErrorStack][memory][input]")
{
    ErrorStack stack{.len = 1};

    CHECK_THAT(addRErrorMsg(nullptr, 0, ""),
               equals_irods_error(USER__NULL_INPUT_ERR));

    CHECK_THAT(freeRError(nullptr),
               equals_irods_error(USER__NULL_INPUT_ERR));

    CHECK_THAT(freeRErrorContent(nullptr),
               equals_irods_error(USER__NULL_INPUT_ERR));

    CHECK_THAT(freeRErrorContent(&stack),
               equals_irods_error(USER__NULL_INPUT_ERR));

    CHECK_THAT(replErrorStack(nullptr, nullptr),
               equals_irods_error(USER__NULL_INPUT_ERR));

    CHECK_THAT(replErrorStack(&stack, nullptr),
               equals_irods_error(USER__NULL_INPUT_ERR));

    CHECK_THAT(replErrorStack(nullptr, &stack),
               equals_irods_error(USER__NULL_INPUT_ERR));
}

TEST_CASE("pop message", "[ErrorStack][memory][input]")
{
    ErrorStack stack{};
    CHECK(irods::pop_error_message(stack) == "");

    stack.len = MAX_ERROR_MESSAGES;
    CHECK(irods::pop_error_message(stack) == "");

    stack.len = MAX_ERROR_MESSAGES + 1;
    CHECK(irods::pop_error_message(stack) == "");

    stack.len = 1;
    CHECK(irods::pop_error_message(stack) == "");
}
