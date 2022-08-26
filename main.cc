#include <iostream>
#include <thread>
#include <future>
#include <string>
#include <vector>
#include <libevdev-1.0/libevdev/libevdev.h>
#include "joycon.hpp"

using namespace std;

int select_device(string prompt) {
    vector<pair<string, string>> list = JoyconUtils::list();
    int i = 0;
    for (pair<string, string> dev : list) {
        cout << "[" << i << "] " << dev.first << ": " << dev.second << endl;
        i++;
    }
    cout << "Hint: Don\'t pick Joycon devices ending in \"IMU\"" << endl;
    cout << prompt << " (0-" << i - 1 << "): ";
    int n;
    cin >> n;
    if (n > (i - 1) || n < 0) {
        cerr << "Invalid choice" << endl;
        return 1;
    }
    string event = list[n].first;
    cout << "Chosen " << event << " (" << list[n].second << ")" << endl;
    return n;
}

int main() {
    int l_n = select_device("Pick device for left Joycon");
    int r_n = select_device("Pick device for right Joycon");
    thread l_th([](int n) {
        Joycon l(n, "Left: ");
        JoyconUtils::add_joycon(&l);
        if (!l.ok()) exit(1);
        else {
            l.loop();
        }
    }, l_n);
    thread r_th([](int n) {
        Joycon r(n, "Right: ");
        JoyconUtils::add_joycon(&r);
        if (!r.ok()) exit(1);
        else {
            struct libevdev *dev = r.get_dev();
            r.loop();
        }
    }, r_n);
    /*sleep(20);
    stop = true;
    JoyconUtils::wait_until_done();*/
    while (true) {};
    return 0;
}