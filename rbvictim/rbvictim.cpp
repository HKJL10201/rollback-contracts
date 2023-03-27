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
    // victim contract
    class [[eosio::contract]] rbvictim : public eosio::contract
    {
    public:
        using eosio::contract::contract;

        // the main gambling logic: if the memo is "win", the player win;
        // otherwise, the player lose
        [[eosio::action]] void playing(eosio::name from, std::string memo)
        {
            print("playing");
            // win the game
            if (memo == "win")
            {
                print("win");
                uint8_t p = 4;
                uint8_t num = 2;
                // using inline call to eosio.token's transfer
                action(
                    permission_level{get_self(), "active"_n},
                    "eosio.token"_n,
                    "transfer"_n,
                    std::make_tuple(get_self(),
                                    from,
                                    asset(num * pow(10, p), symbol("SYS", p)),
                                    std::string("you win")))
                    .send();
            }
            // lose the game
            else
            {
                print("lose");
                uint8_t p = 4;
                uint8_t num = 1;
                // using inline call to eosio.token's transfer
                action(
                    permission_level{get_self(), "active"_n},
                    "eosio.token"_n,
                    "transfer"_n,
                    std::make_tuple(get_self(),
                                    from,
                                    asset(num * pow(10, p), symbol("SYS", p)),
                                    std::string("you lose")))
                    .send();
            }
        }
    };
}

// handle the notification sent by eosio.token's transfer
void on_transfer(const transfer_data &transfer)
{
    // if the victim is the receiver of transfer
    if (transfer.to == "rbvictim"_n)
    {
        print(transfer.from, " received ", transfer.quantity.to_string(), " from ", transfer.from.to_string());
        // inline call: call the playing action defined above
        action(
            permission_level{"rbvictim"_n, "active"_n},
            "rbvictim"_n, "playing"_n,
            std::make_tuple(transfer.from, transfer.memo))
            .send();
    }
    // if the victim is the sender of transfer
    else if (transfer.from == "rbvictim"_n)
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
            if (action == name("playing").value)
            {
                eosio::execute_action(eosio::name(receiver), eosio::name(code), &rbvictim::playing);
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
