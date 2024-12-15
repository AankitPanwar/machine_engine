/* 
following steps to run the below code 
1) g++ -std=c++11 -pthread -o machine machine_eng.cpp ;  
2) ./machine
*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
using namespace std;

// Define message structures
struct PlaceOrder {
    int orderId;
    int price;
    int amount;
    bool isBuyOrder; // True for buy, false for sell
};

struct CancelOrder {
    int orderId;
};

class MatchingEngine {
private:
    std::vector<PlaceOrder> buyOrders; // Buy orders 
    std::vector<PlaceOrder> sellOrders; // Sell orders 
    std::mutex engineMutex;

    // Sort buy orders in descending order of price , so that we got the maximum price at the first.
    void sortBuyOrders() {
        std::sort(buyOrders.begin(), buyOrders.end(), [](const PlaceOrder& a, const PlaceOrder& b) {
            return a.price > b.price;
        });
    } 

    // Sort sell orders in ascending order of price , so we got the minimum price at the first.
    void sortSellOrders() {
        std::sort(sellOrders.begin(), sellOrders.end(), [](const PlaceOrder& a, const PlaceOrder& b) {
            return a.price < b.price;
        });
    }

    // Match orders between buyers and sellers
    void matchOrders() {
        while (!buyOrders.empty() && !sellOrders.empty()) 
        {
            PlaceOrder& buy = buyOrders.front();    
            PlaceOrder& sell = sellOrders.front();  

            if (buy.price >= sell.price) // Check if prices match 
            { 
                int tradeAmount = std::min(buy.amount, sell.amount);
                int tradePrice = sell.price; // this is the final trade Price , seller want to sel;
                std::cout << "Trade executed:  BuyOrder (" << buy.orderId << ") and SellOrder (" << sell.orderId  << ")  at Price: " << tradePrice  
                << " for Amount: " << tradeAmount << std::endl;

                // once the trade successfull , reduce the trade amount from the buyers and seller price.
                buy.amount  -= tradeAmount;  
                sell.amount -= tradeAmount; 

              if (buy.amount == 0)  { buyOrders.erase(buyOrders.begin()); } 
              if (sell.amount == 0) { sellOrders.erase(sellOrders.begin()); }

            } 
            else
             {
                break; // No more matches possible
            }
        }
    }

public:
    // Place an order into the matching engine
    void placeOrder(const PlaceOrder& order) 
    {
        /* this is the critical section for the code  , so it has to mutually shared*/
        std::lock_guard<std::mutex> lock(engineMutex);
        // to check if the placed order is from buyer or not 
        if (order.isBuyOrder)   // yes from buyers , then push the details into the buyers vector
        {
            buyOrders.push_back(order);
            sortBuyOrders();
        } 
        else                // No , then it's seller order, then push the details to the seller vector 
        {
            sellOrders.push_back(order);
            sortSellOrders();
        }

/*    std::cout << "************* BUYER ****************" << std::endl;
    for(auto buy_value : buyOrders)
    {
                std::cout << "Buyer OrderID = " << buy_value.orderId  << std::endl;
                std::cout << "Buyer Price   = " << buy_value.price << std::endl;
                std::cout << "Buyer Amount  = " << buy_value.amount << std::endl;
        std::cout << "*****************************" << std::endl;
    }
    std::cout <<std::endl;

    for(auto buy_value : sellOrders)
    {
                std::cout << "Seller OrderID = " << buy_value.orderId  << std::endl;
                std::cout << "Seller Price   = " << buy_value.price << std::endl;
                std::cout << "Seller Amount  = " << buy_value.amount << std::endl;
        std::cout << "*****************************" << std::endl;
    }*/
      std::cout <<std::endl;

        matchOrders();
    }

    // Cancel an order from the matching engine
    void cancelOrder(const CancelOrder& cancel) 
    {
        // Lock the mutex for the concurrent process

        // Find the orderId in placeOrder structure , 
        // If if matches , then print the the  cancelled message , erase from for the PlaceOrdered.
        std::lock_guard<std::mutex> lock(engineMutex);

        auto removeOrder = [&](std::vector<PlaceOrder>& orders) {
            auto it = std::remove_if(orders.begin(), orders.end(), [&](const PlaceOrder& order) {
                return order.orderId == cancel.orderId;
            });

            if (it != orders.end()) {
                orders.erase(it, orders.end());
                std::cout << "Order " << cancel.orderId << " canceled." << std::endl;
                return true;
            }
            return false;
        };

        if (!removeOrder(buyOrders) && !removeOrder(sellOrders)) {
            std::cout << "Order " << cancel.orderId << " not found." << std::endl;
        }
    }
};

void clientFunction(MatchingEngine& engine, int clientId) 
{ 
    for (int i = 0; i < 3; ++i) 
    {
        PlaceOrder order
        {
            clientId * 10 + i,  // clientID
            100 + i * 10,       // price
            10 + i,         //amount
            clientId % 2 == 0  // is it buyOrder or not ,( even client id are buyers , odd are sellers )
        };

        engine.placeOrder(order);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    MatchingEngine engine;

    // Start test clients in separate threads
    std::thread sellerThread(clientFunction, std::ref(engine), 1);
    std::thread buyerThread(clientFunction,  std::ref(engine), 2);

    sellerThread.join();
    buyerThread.join();

    return 0;
}
