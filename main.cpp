
#include "car_rental.h"
#include <iomanip>
#include <iostream>

using namespace std;

static string readLine(const string& prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

static int readInt(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        try { size_t p; int v = stoi(s, &p); if (p == s.size()) return v; }
        catch (...) {}
        cout << "Please enter an integer.\n";
    }
}

static double readDouble(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        try { size_t p; double v = stod(s, &p); if (p == s.size() && v >= 0) return v; }
        catch (...) {}
        cout << "Please enter a non-negative number.\n";
    }
}

static void printOrder(const Order& o) {
    cout << "Order #" << o.id << " | " << o.car->name() << " | " << o.clientName
        << " | passport: " << o.passport << " | days: " << o.days << "\n   Status: "
        << statusToString(o.status);
    if (o.status == OrderStatus::Rejected) cout << " (reason: " << o.rejectReason << ")";
    cout << "\n   Rent: " << fixed << setprecision(2) << o.rentCost
        << " UAH, damage: " << o.damageCost << " UAH, TOTAL: " << o.total() << " UAH\n";
}

static void listCars(RentalSystem& sys, bool onlyAvailable) {
    cout << "\n--- Cars ---\n";
    for (const auto& c : sys.cars()) {
        if (onlyAvailable && !c->isAvailable()) continue;
        cout << c->describe() << (c->isAvailable() ? "" : "  (rented)") << "\n";
        for (const auto& d : c->damages()) cout << "     damage: " << d << "\n";
    }
}
static void demoPolymorphism(RentalSystem& sys) {
    int days = readInt("Calculate the cost for how many days? ");
    cout << "\n--- Polymorphic calls (Car*) ---\n";
    for (const auto& c : sys.cars()) {
        const Car* p = c.get();
        cout << setw(12) << left << p->category() << setw(22) << p->name()
            << right << fixed << setprecision(2) << p->calculateRent(days) << " UAH\n";
    }
}

static void demoUnaryOperators() {
    cout << "\n--- Unary operators on cars (rate step = " << Car::kRateStep << " UAH) ---\n";
    EconomyCar eco(100, "Demo", "Economy", "DEMO01", 800);
    BusinessCar biz(101, "Demo", "Business", "DEMO02", 2000);
    SUVCar suv(102, "Demo", "SUV", "DEMO03", 2800);

    cout << "Start:        " << eco.baseRate() << " | " << biz.baseRate() << " | "
        << suv.baseRate() << "\n";
    ++eco; ++biz; ++suv;
    cout << "After ++x:    " << eco.baseRate() << " | " << biz.baseRate() << " | "
        << suv.baseRate() << "\n";
    EconomyCar oldEco = eco++;
    cout << "Postfix eco++ returned old rate " << oldEco.baseRate()
        << ", new rate is " << eco.baseRate() << "\n";
    --biz;
    BusinessCar oldBiz = biz--;
    cout << "Prefix --biz, then postfix biz--: old " << oldBiz.baseRate()
        << ", new " << biz.baseRate() << "\n";
    suv--;
    cout << "After suv--:  " << suv.baseRate() << "\n";

    cout << "\n--- Unary operators on an order ---\n";
    Order order;
    order.id = 1;
    order.car = &eco;
    order.clientName = "Demo Client";
    order.passport = "XX000000";
    order.days = 3;
    order.rentCost = eco.calculateRent(order.days);
    cout << "Start:      days = " << order.days << ", rent = " << order.rentCost << "\n";
    ++order;
    cout << "After ++:   days = " << order.days << ", rent = " << order.rentCost << "\n";
    Order oldOrder = order--;
    cout << "Postfix -- returned old days = " << oldOrder.days
        << ", now days = " << order.days << ", rent = " << order.rentCost << "\n";
    order.status = OrderStatus::Approved;
    ++order;
    cout << "Approved order is not changed by ++: days = " << order.days << "\n";
}

static void clientMenu(RentalSystem& sys) {
    while (true) {
        cout << "\n=== CLIENT ===\n1. Available cars\n2. Create an order\n"
            "3. Pay for an order\n4. View orders\n5. Change rental period of a pending order\n0. Back\n";
        int ch = readInt("> ");
        string err;
        if (ch == 0) return;
        if (ch == 1) listCars(sys, true);
        else if (ch == 2) {
            listCars(sys, true);
            int id = readInt("Car ID: ");
            string name = readLine("Full name: ");
            string passport = readLine("Passport data (series/number): ");
            int days = readInt("Rental period (days): ");
            int oid = sys.createOrder(id, name, passport, days, err);
            if (oid < 0) cout << "Error: " << err << "\n";
            else cout << "Request #" << oid << " submitted. Wait for the administrator's decision.\n";
        }
        else if (ch == 3) {
            int oid = readInt("Order number: ");
            if (sys.pay(oid, err)) {
                cout << "Payment accepted. Amount invoiced: " << fixed << setprecision(2)
                    << sys.findOrder(oid)->rentCost << " UAH.\n";
            }
            else cout << "Error: " << err << "\n";
        }
        else if (ch == 4) {
            if (sys.orders().empty()) cout << "No orders.\n";
            for (const auto& o : sys.orders()) printOrder(o);
        }
        else if (ch == 5) {
            int oid = readInt("Order number: ");
            Order* o = sys.findOrder(oid);
            if (!o) { cout << "Error: Order not found.\n"; continue; }
            if (o->status != OrderStatus::Pending) {
                cout << "Error: Only a pending order can be changed.\n";
                continue;
            }
            string op = readLine("Enter + to extend by one day or - to shorten: ");
            if (op == "+") ++(*o);
            else if (op == "-") --(*o);
            else { cout << "Unknown choice.\n"; continue; }
            printOrder(*o);
        }
    }
}

static void adminMenu(RentalSystem& sys) {
    while (true) {
        cout << "\n=== ADMINISTRATOR ===\n1. All orders\n2. Approve a request\n"
            "3. Reject a request\n4. Accept returned car (damage notes)\n"
            "5. All cars\n0. Back\n";
        int ch = readInt("> ");
        string err;
        if (ch == 0) return;
        if (ch == 1) {
            if (sys.orders().empty()) cout << "No orders.\n";
            for (const auto& o : sys.orders()) printOrder(o);
        }
        else if (ch == 2) {
            int oid = readInt("Request number: ");
            if (sys.approve(oid, err)) cout << "Request approved.\n";
            else cout << "Error: " << err << "\n";
        }
        else if (ch == 3) {
            int oid = readInt("Request number: ");
            string reason = readLine("Reason for rejection: ");
            if (sys.reject(oid, reason, err)) cout << "Request rejected.\n";
            else cout << "Error: " << err << "\n";
        }
        else if (ch == 4) {
            int oid = readInt("Order number: ");
            string note = readLine("Damage description (empty = no damage): ");
            double fee = note.empty() ? 0 : readDouble("Compensation amount (UAH): ");
            if (sys.returnCar(oid, note, fee, err)) {
                cout << "Car accepted. Final amount: " << fixed << setprecision(2)
                    << sys.findOrder(oid)->total() << " UAH.\n";
            }
            else cout << "Error: " << err << "\n";
        }
        else if (ch == 5) listCars(sys, false);
    }
}

int main() {
    RentalSystem sys;
    sys.addCar(make_unique<EconomyCar>(1, "Skoda", "Fabia", "AA1111AA", 800));
    sys.addCar(make_unique<EconomyCar>(2, "Renault", "Logan", "AA2222BB", 700));
    sys.addCar(make_unique<BusinessCar>(3, "BMW", "520", "AA3333CC", 2000));
    sys.addCar(make_unique<BusinessCar>(4, "Audi", "A6", "AA4444DD", 2200));
    sys.addCar(make_unique<SUVCar>(5, "Toyota", "Land Cruiser", "AA5555EE", 2800));

    while (true) {
        cout << "\n===== CAR RENTAL =====\n1. I am a client\n2. I am an administrator\n"
            "3. Polymorphism demo\n4. Unary operators demo (++ / --)\n0. Exit\n";
        int ch = readInt("> ");
        if (ch == 0) break;
        if (ch == 1) clientMenu(sys);
        else if (ch == 2) adminMenu(sys);
        else if (ch == 3) demoPolymorphism(sys);
        else if (ch == 4) demoUnaryOperators();
    }
    return 0;
}
