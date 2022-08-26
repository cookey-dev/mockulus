#pragma once

#define _XOPEN_SOURCE 700
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <filesystem>
#include <boost/regex.hpp>
#include <linux/input-event-codes.h>
#include <boost/algorithm/string_regex.hpp>

using namespace std;
namespace fs = std::filesystem;
namespace algo = boost::algorithm;

atomic_bool stop = false;
atomic_bool done = false;

class Joycon {
public:
    Joycon(int event, string prompt, bool quiet = false) {
        is_quiet = quiet;
        pr = prompt;
        open_event(event);
        if (fd == -1 || rc < 0) {
            cerr << pr << "Joycon: error opening event (" << strerror(-rc) << ")" << endl;
            is_ok = false;
        } else {
            libevdev_enable_property(dev, 0x40);
            if (!is_quiet) cout << info();
        }
    }
    ~Joycon() {
        libevdev_free(dev);
    }
    bool ok() {
        return is_ok;
    }
    bool is_done;
    string info() {
        ostringstream res;
        res << pr << "Joycon device name: \"" << libevdev_get_name(dev) << "\"\n"
            << pr << "Joycon bus id: " << libevdev_get_id_bustype(dev) << "\n"
            << pr << "Joycon vendor id: " << libevdev_get_id_vendor(dev) << "\n"
            << pr << "Joycon product id: " << libevdev_get_id_product(dev) << "\n";
            //<< pr << "Joycon abs: " << libevdev_get_abs_info(dev, ) << "\n";
        return res.str();
    }
    struct libevdev *get_dev() {
        return dev;
    }
    void loop() {
        do {
            struct input_event ev;
            rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (rc == 0) {
                cout << pr << "Event type: " << libevdev_event_type_get_name(ev.type) << endl;
                cout << pr << "Event code: " << libevdev_event_code_get_name(ev.type, ev.code) << endl;
                cout << pr << "Event value: " << ev.value << endl;
            }
        } while ((rc == 1 || rc == 0 || rc == -EAGAIN) && !stop);
        is_done = true;
    }
private:
    int fd;
    int rc = 1;
    struct libevdev *dev = NULL;
    bool is_ok = true;
    bool is_quiet;
    string pr;
    void open_event(int event) {
        if (!is_quiet) cout << pr << "Joycon: opening event" << endl;
        ostringstream event_str_s;
        event_str_s << "/dev/input/event" << event;
        string event_str = event_str_s.str();
        if (!is_quiet) cout << pr << "Joycon: Picked event " << event_str << endl;
        fd = open(event_str.c_str(), O_RDONLY | O_NONBLOCK);
        rc = libevdev_new_from_fd(fd, &dev);
    }
};

namespace JoyconUtils {
    namespace _ {
        string get_device_name(string path) {
            struct libevdev *dev;
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            int rc = libevdev_new_from_fd(fd, &dev);
            string name = "";
            if (!(fd == -1 || rc < 0)) {
                name = libevdev_get_name(dev);
                libevdev_free(dev);
            }
            return name;
        }
        bool is_event(string path) {
            fs::path p(path);
            string fname = p.filename();
            return fname.find("event") != string::npos;
        }
        int get_event_id(string path) {
            vector<string> res;
            algo::split_regex(res, path, boost::regex("event"));
            return stoi(res[1]);
        }
        bool list_cmp(pair<string, string> a, pair<string, string> b) {
            return get_event_id(a.first) < get_event_id(b.first);
        }
        vector<Joycon*> joycons;
    }
    vector<pair<string, string>> list() {
        map<string, string> files;
        DIR *dir;
        struct dirent *e;
        dir = opendir("/dev/input");
        if (dir != NULL) {
            while ((e = readdir(dir)) != NULL) {
                ostringstream _d_name;
                _d_name << "/dev/input/" << e->d_name;
                string d_name = _d_name.str();
                if (_::is_event(d_name)) {
                    string dev_name = _::get_device_name(d_name);
                    if (dev_name != "") files.insert({d_name, dev_name});
                }
            }
            closedir(dir);
        }
        vector<pair<string, string>> files_vec;
        for (pair<string, string> i : files) {
            files_vec.push_back(i);
        }
        sort(files_vec.begin(), files_vec.end(), _::list_cmp);
        return files_vec;
    }
    void add_joycon(Joycon *j) {
        _::joycons.push_back(j);
    }
    void wait_until_done() {
        vector<bool> wait;
        while (wait.size() != _::joycons.size()) {
            int i = 0;
            for (Joycon *j : _::joycons) {
                if (i <= (wait.size() - 1)) continue;
                if (j->is_done) wait.push_back(true);
                i++;
            }
        }
    }
}
