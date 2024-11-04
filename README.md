# Deribit Trading System - Comprehensive Order Management and Execution Platform (C.O.M.E.P.) Using Deribit API
## Overview
DeribitTradingSystem is a powerful C++ application designed to interface with the Deribit cryptocurrency trading platform, enabling streamlined trading operations. This system supports a variety of trading functions, including placing, modifying, and canceling orders, as well as managing positions and retrieving order book data. Additionally, a WebSocket server allows clients to subscribe to specific symbols for continuous real-time order book updates.

## Demonstration Video
Watch the full system functionality demonstration: [Watch Video](https://drive.google.com/file/d/1Sjf_krFRaGcYWY3W12_9CwuhtGmyqg6b/view?usp=sharing)

## Features
- **Place Orders**: Place buy and sell orders on Deribit.
- **Modify Orders**: Provides options to adjust, cancel, or modify orders. You can:
  - Cancel orders by ID, currency, currency pair, instrument, label, or order type.
  - Edit orders by ID or label.
- **Retrieve Order Book**: Fetch and view the order book for specific trading pairs.
- **View Current Positions**: Display all active trading positions.
- **WebSocket Server**: Enables external clients to connect, subscribe to symbols, and receive real-time order book updates.

## Scope
- Supports Deribit futures, options, and perpetual contracts.
- Provides low-latency order execution and market data access.
- Allows external WebSocket clients to connect, subscribe to a symbol, and receive continuous order book updates.

## Prerequisites
- *C++ Compiler*: C++17 or later
- *MSYS2*: Installed and configured for building and running C++ applications

### Dependencies
- *WebSocket++*: For WebSocket client/server functionality
- *cURL*: For HTTP requests
- *OpenSSL*: For secure connections
- *JsonCpp*: For JSON parsing

## Installation
### Step 1: Clone the Repository
```bash
git clone https://github.com/bhaskar-nie/DeribitTradingSystem.git
cd DeribitTradingSystem
```

### Step 2: Install Dependencies
Ensure MSYS2 is installed and properly configured. Install Required Packages:
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-curl mingw-w64-x86_64-openssl mingw-w64-x86_64-jsoncpp mingw-w64-x86_64-websocketpp
```

### Step 3: Set Up API Credentials
Create a api_credentials.txt file in the config/ directory with the following content:

```bash
clientId = YOUR_CLIENT_ID
clientSecret = YOUR_CLIENT_SECRET
```
Replace YOUR_CLIENT_ID and YOUR_CLIENT_SECRET with your [Deribit API](https://test.deribit.com/) credentials.

### Step 4: Build the Project
Use the following command to compile the application:
```bash
g++ -o DeribitTradingSystem src/main.cpp src/api.cpp src/websocket_server.cpp src/websocket_client.cpp -I include -lcurl -ljsoncpp -lpthread -lws2_32 -lmswsock
```

### Step 5: Run the Project
Execute the program using:
```bash
./DeribitTradingSystem
```

### Usage

*Example Input for Each Action*

After running the application, you can interact with it using the following commands:

1. *Authenticate and Initialize*
   - The application will automatically prompt you for your API credentials from api_credentials.txt.

2. *Place Buy Order*
   - *Currency:* BTC
   - *Kind:* future
   - *Symbol:* BTC-PERPETUAL
   - *Amount:* 1.0
   - *Label:* MyBuyOrder
   - *Order Type:* market

3. *Place Sell Order*
   - *Currency:* ETH
   - *Kind:* future
   - *Symbol:* ETH-PERPETUAL
   - *Amount:* 0.5
   - *Price:* 2500.0
   - *Trigger Type:* last_price
   - *Trigger Price:* 2600.0
   - *Order Type:* stop_limit

4. *Modify Order*
   - *Option to Cancel or Edit Orders:*
     - *Cancel Order by ID*
       - *Order ID:* 1234-5678-ABCD
     - *Cancel All Orders*
     - *Cancel Orders by Currency:* BTC
     - *Cancel Orders by Currency Pair:* BTC_USD
     - *Edit Order by ID*
       - *Order ID:* 1234-5678-ABCD
       - *New Amount:* 0.3
       - *New Price:* 2550.0

5. *Get Order Book*
   - *Get by Instrument Name:*
     - *Instrument Name:* BTC-PERPETUAL
   - *Get by Instrument ID:*
     - *Instrument ID:* 123456

6. *View Current Positions*
   - *Option to Get Position by Instrument Name or All Positions:*
     - *Instrument Name:* BTC-PERPETUAL
     - *Currency:* BTC
     - *Kind:* future

7. *Start WebSocket Server*
   - Command to start the WebSocket server on port 9002.

8. *Subscribe to Symbol for Order Book Updates*
   - *Symbol:* ETH-PERPETUAL
   - Enter the symbol to receive continuous order book updates.

9. *Exit the Application*
   - Command to safely exit the program.


## Environment Variables
Set up the following variables in /src/config.json:
```bash
{
    "clientId": "YOUR_CLIENT_ID",
    "clientSecret": "YOUR_CLIENT_SECRET"
}
```

## Error Handling
The application handles a variety of errors, such as invalid parameters, failed connection attempts, and unauthorized requests. Errors are logged and displayed on the console. For troubleshooting, ensure your credentials are correct and that your MSYS2 environment is set up properly.

 
