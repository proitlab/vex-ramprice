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

        // @abi table pricetab i64
        struct [[eosio::table]] pricetab {
            float_t price;
            uint32_t timestamp;

            uint64_t primary_key() const { return timestamp; }

            EOSLIB_SERIALIZE(pricetab, (price)(timestamp));

        };

        typedef eosio::multi_index< "ramprices"_n, pricetab > ramprices;

        
        [[eosio::action]]
        void getversion() {
            print("RAMPrice Smart Contract v1.2 - 20200727\t");
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
    
            rammarket _rammarket = rammarket("vexcore"_n, "vexcore"_n.value);

            auto itr = _rammarket.find(ramcore_symbol.raw());

            float_t quote_balance = int64_t(itr->quote.balance.amount) / 10000;
            float_t current_ramprice = quote_balance / int64_t(itr->base.balance.amount);


            uint32_t current_timestamp = now();

            ramprices _ramprices(get_self(), get_first_receiver().value);

            auto itr2 = _ramprices.find( current_timestamp );

            if( itr2 == _ramprices.end() ) {
                _ramprices.emplace( "vexessential"_n, [&](auto& row){
                    row.price = current_ramprice;
                    row.timestamp = current_timestamp;
                });
            }

        };
    

};