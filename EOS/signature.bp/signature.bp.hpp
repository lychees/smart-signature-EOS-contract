/**
 *  @dev minakokojima
 *  @copyright Andoromeda
 */
#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>

#include "config.hpp"
#include "model/Contract/EOS/util/util.hpp"
#include "council.hpp"
#include "kyubey.hpp"
#include "NFT.hpp"

using namespace eosio ;
using namespace config ;
using namespace kyubeytool ;

CONTRACT sign : public eosio::contract {
    public: 
        sign( name receiver, name code, datastream<const char*> ds ) :
        /*council(self),*/
        _global(_self, _self),
        _market(_self, _self),
        _signs(_self, _self){}

    struct [[eosio::table("signs")]] sign_info {
        uint64_t id;
        name creator = 0;
        name owner = 0;
        uint64_t creator_fee;
        uint64_t ref_fee;
        uint64_t k;        
        uint64_t price;
        uint64_t last_anti_bot_fee = 0;
        uint64_t anti_bot_init_fee;
        time anti_bot_timer;
        time last_buy_timer;        
        time st;
        uint64_t primary_key()const { return id; }
        uint64_t next_price() const {
            return price * k / 1000;
        }
    };    
    
    struct [[eosio::table("players")]] player_info {
        vector<uint64_t> signs;
        asset ref_income;
        asset staked_income;
        asset article_income;
        asset sponsor_income;       
    };
        
    struct [[eosio::table("global")]] st_global {
        uint64_t defer_id;
        uint64_t total_staked;
        uint64_t global_fee;
        name last;
        time st, ed;
    };

    typedef singleton<"global"_n, st_global> singleton_global_t;
    typedef eosio::multi_index<"signs"_n, sign_info> sign_index_t;
    typedef eosio::multi_index<"market"_n, kyubey::market> market_index_t;
    typedef singleton<"players"_n, player_info> singleton_players_t;  
    
    // Contract management
    ACTION init();
    ACTION clear();     
    
    ACTION unstake(name from, uint64_t amount);
    ACTION claim(name from);    

    ACTION airdrop(name to, uint64_t amount);

    ACTION transfer(name   from,
                  name   to,
                  asset          quantity,
                  string         memo);

    ACTION test();

    ACTION receipt(const rec_reveal& reveal) {
        require_auth(_self);
    }

    // new
    ACTION signcreate( name owner ) {
        require_auth(_self);
        
        //two-way binding.
        auto itr_p = _players.require_find( owner, "Unable to find player" );
        auto itr_newsign = _signs.emplace( _self, [&](auto &s) {
            s.id = _signs.available_primary_key();
            // s.creator
            s.owner = owner;

            /*
            uint64_t creator_fee;
            uint64_t ref_fee;
            uint64_t k;        
            uint64_t price;
            uint64_t last_anti_bot_fee = 0;
            uint64_t anti_bot_init_fee;
            time anti_bot_timer;
            time last_buy_timer;        
            time st;
            */
        });

        _players.modify(itr_p, _self, [&](auto &p) {
            p.signs.push_back(itr_newsign->id);
        });
    }

    void apply(uint64_t receiver, uint64_t code, uint64_t action);

    // defer_action
    uint64_t get_next_defer_id() {
        auto g = _global.get();    
        g.defer_id += 1;
        _global.set(g, _self);
        return g.defer_id;
    }

    template <typename... Args>
    void send_defer_action(Args&&... args) {
        transaction trx;
        trx.actions.emplace_back(std::forward<Args>(args)...);
        trx.send(get_next_defer_id(), _self, false);
    }

private:
    singleton_global_t _global; 
    sign_index_t _signs;
    market_index_t _market;    
    

    void onTransfer(name from, name to,
                    extended_asset quantity, string memo); 

    void create(name from, extended_asset in, const vector<string>& params);
    void sponsor(name from, extended_asset in, const vector<string>& params);    
    void buy(name from, extended_asset in, const vector<string>& params);
    void sell(name from, extended_asset in, const vector<string>& params);

};


void sign::apply(uint64_t receiver, uint64_t code, uint64_t action) {   
    auto &thiscontract = *this;
    if (action == ( "transfer"_n ).value) {
        auto transfer_data = unpack_action_data<kyubeyutil::st_transfer>();
        onTransfer(transfer_data.from, transfer_data.to, extended_asset(transfer_data.quantity, code), transfer_data.memo);               
        return;
    }

    if (code != get_self().value) return;
    switch (action) {
        EOSIO_DISPATCH_HELPER(sign,
                              (init))
    }
}

extern "C" { // renew done
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) 
    {
        sign p( name(receiver), name(code), datastream<const char*>(nullptr, 0) );
        p.apply(receiver, code, action);
        eosio_exit(0);
    }
}