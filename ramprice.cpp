#include <eosio/asset.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using namespace eosio;

class [[eosio::contract]] ramprice : public eosio::contract {

    private:
         static constexpr symbol ramcore_symbol = symbol(symbol_code("RAMCORE"), 4);
         static constexpr symbol ram_symbol     = symbol(symbol_code("RAM"), 0);

        /*
        Table rammarket struct
        */
        struct exchange_state {
            asset    supply;

            struct connector {
                asset balance;
                double weight = .5;

                EOSLIB_SERIALIZE( connector, (balance)(weight) )
            };

            connector base;
            connector quote;

            uint64_t primary_key()const { return supply.symbol.raw(); }

            EOSLIB_SERIALIZE( exchange_state, (supply)(base)(quote) )
        };

        typedef eosio::multi_index< "rammarket"_n, exchange_state > rammarket;


        
        uint32_t now() {
            return current_time_point().sec_since_epoch();
        }
        

   public:
        using contract::contract;

        // @abi table pricetab
        struct [[eosio::table]] pricetab {
            float_t price;
            uint32_t timestamp;

            uint64_t primary_key() const { return timestamp; }

            EOSLIB_SERIALIZE(pricetab, (price)(timestamp));

        };

        typedef eosio::multi_index< "ramprices"_n, pricetab > ramprices;

        // @abi table traders
        struct [[eosio::table]] traders {
            name account_name;
            asset buy_quantity;
            asset sell_quantity;
            asset ram_buy_quantity;
            asset ram_sell_quantity;
            uint32_t buy_counter;
            uint32_t sell_counter;
 
            uint64_t primary_key() const { return account_name.value; }

            EOSLIB_SERIALIZE(traders, (account_name)(buy_quantity)(sell_quantity)
            (ram_buy_quantity)(ram_sell_quantity)(buy_counter)(sell_counter));

        };

        typedef eosio::multi_index<"ramtraders"_n, traders> ramtraders;

        [[eosio::action]]
        void getversion() {
            print("RAMPrice SC v1.3 - proitidgovex - 20200727\t");
            rammarket _rammarket = rammarket("vexcore"_n, "vexcore"_n.value);

            auto itr = _rammarket.find(ramcore_symbol.raw());

            float_t quote_balance = int64_t(itr->quote.balance.amount) / 10000;
            float_t current_ramprice = quote_balance / int64_t(itr->base.balance.amount);

            print("Base Balance:", itr->base.balance, "\t");
            print("Quote Balance:", itr->quote.balance, "\t");
            print("Current Price:", current_ramprice, "\t");
        }
   
        [[eosio::on_notify("vex.ram::transfer")]]
        void insprice(name from, name to, eosio::asset quantity, std::string memo) {
    
            name trader;
            bool is_buy;
            uint64_t ram_amount;

            rammarket _rammarket = rammarket("vexcore"_n, "vexcore"_n.value);

            auto itr1 = _rammarket.find(ramcore_symbol.raw());

            float_t quote_balance = int64_t(itr1->quote.balance.amount) / 10000;
            float_t current_ramprice = quote_balance / int64_t(itr1->base.balance.amount);


            uint32_t current_timestamp = now();

            ramprices _ramprices(get_self(), get_first_receiver().value);

            auto itr2 = _ramprices.find( current_timestamp );

            if( itr2 == _ramprices.end() ) {
                _ramprices.emplace( "vexcore"_n, [&](auto& row){
                    row.price = current_ramprice;
                    row.timestamp = current_timestamp;
                });
            }

            if ( memo == "buy ram") {
                trader = from;
                is_buy = true;
            } else {
                trader = to;
                is_buy = false;
            }

            ramtraders _ramtraders(get_self(), get_self().value);

            auto itr3 = _ramtraders.find( trader.value );

            if( itr3 == _ramtraders.end() ) {

                /* trader is NOT in table ramtraders */
                _ramtraders.emplace(trader, [&](auto& row) {
                    row.account_name = trader;
                    ram_amount = (uint64_t) (quantity.amount / current_ramprice);
                    if( is_buy ) {
                        row.buy_quantity = quantity;
                        row.buy_counter++;
                        row.ram_buy_quantity.amount = ram_amount;
                        row.ram_buy_quantity.symbol = ram_symbol;
                        
                    } else {
                        row.sell_quantity = quantity;
                        row.sell_counter++;
                        row.ram_sell_quantity.amount = ram_amount;
                        row.ram_sell_quantity.symbol = ram_symbol;
                    }

                });
 
            } else {
                /* trader is in table ramtraders */
                _ramtraders.modify( itr3, trader, [&](auto& row){
                    row.account_name = trader;
                    ram_amount = (uint64_t) (quantity.amount / current_ramprice);
                    if( is_buy ) {
                        row.buy_quantity += quantity;
                        row.buy_counter++;
                        row.ram_buy_quantity.amount += ram_amount;
                        row.ram_buy_quantity.symbol = ram_symbol;
                        
                    } else {
                        row.sell_quantity += quantity;
                        row.sell_counter++;
                        row.ram_sell_quantity.amount += ram_amount;
                        row.ram_sell_quantity.symbol = ram_symbol;
                    }
                });
            }

        };
    

};