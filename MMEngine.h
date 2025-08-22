
                                                 // MM1Engine.h ¡X header-only, works with classic bcc32 (no <random>, no STL deps)
// Drop this file in your project and `#include "MM1Engine.h"` in Unit5.cpp.
// The engine is UI-agnostic; you can feed distributions via callbacks and
// receive simulation updates via a small observer interface.

#pragma once

#include <cmath>
#include <cstdlib>

namespace mm1 {

// -------------------------- Observer (optional) ---------------------------
class ISimObserver {
public:
    virtual ~ISimObserver() {}
    // Called whenever we plot the instantaneous queue size (Series1)
    virtual void onPoint(double /*t*/, int /*queue_size*/) {}
    // Called when mean values update (Series2)
    virtual void onMean(double /*t*/, double /*meanQ*/, double /*meanS*/) {}
    // Called every loop iteration (optional, e.g., to update a time textbox)
    virtual void onStep(double /*t*/, int /*q*/, int /*s*/, double /*meanQ*/, double /*meanS*/) {}
};

// ------------------------- Future Event List (FEL) ------------------------
// A tiny sorted singly-linked list by event time (like your current code)
struct EventNode {
    int type;         // 0=ARRIVAL, 1=SERVICE, 2=DEPARTURE
    double time;      // event time
    EventNode* next;  // next node
    EventNode(int t = 0, double tm = 0.0) : type(t), time(tm), next(0) {}
};

class FEL {
public:
    FEL() : head_(new EventNode(-1, 0.0)) {}
    ~FEL() { clear(); delete head_; }

    void push(int type, double time) {
        EventNode* n = new EventNode(type, time);
        EventNode* cur = head_;
        while (cur->next && cur->next->time <= time) cur = cur->next;
        n->next = cur->next; cur->next = n;
    }

    bool pop(int &type, double &time) {
        if (!head_->next) return false;
        EventNode* n = head_->next; head_->next = n->next;
        type = n->type; time = n->time; delete n; return true;
    }

    bool empty() const { return head_->next == 0; }

    void clear() { int t; double tm; while (pop(t, tm)) {} }

private:
    EventNode* head_; // dummy head
};

// ------------------------- Distribution callback --------------------------
// Provide generator callbacks so the engine does not depend on any RNG lib.
// Return next inter-arrival or service time in SECONDS (>0 ideally).
// `user` is a cookie pointer to pass parameters (e.g., mean, stddev, etc.)

typedef double (*SampleFn)(void* user);

// ------------------------------- Engine ----------------------------------
class Engine {
public:
    enum { ARRIVAL = 0, SERVICE = 1, DEPARTURE = 2 };

    Engine() :
        end_time_(0.0), now_(0.0), prev_(0.0),
        queue_(0), system_(0), sumQ_(0.0), sumS_(0.0),
        meanQ_(0.0), meanS_(0.0),
        next_event_(ARRIVAL), next_time_(0.0),
        obs_(0), inter_fn_(0), inter_user_(0), serv_fn_(0), serv_user_(0) {}

    void setObserver(ISimObserver* obs) { obs_ = obs; }

    void setInterArrival(SampleFn fn, void* user) { inter_fn_ = fn; inter_user_ = user; }
    void setService    (SampleFn fn, void* user) { serv_fn_  = fn; serv_user_  = user; }

    void reset() {
        now_ = 0.0; prev_ = 0.0; end_time_ = 0.0;
        queue_ = 0; system_ = 0;
        sumQ_ = 0.0; sumS_ = 0.0; meanQ_ = 0.0; meanS_ = 0.0;
        next_event_ = ARRIVAL; next_time_ = 0.0;
        fel_.clear();
    }

    void run(double end_time) {
        end_time_ = end_time;
        // Main loop
        while (now_ < end_time_) {
            switch (next_event_) {
                case ARRIVAL:   arrival();   break;
                case SERVICE:   service();   break;
                case DEPARTURE: departure(); break;
            }
            if (obs_) obs_->onStep(now_, queue_, system_, meanQ_, meanS_);
        }
    }

    // Accessors if you need
    double now() const { return now_; }
    int    queue() const { return queue_; }
    int    system() const { return system_; }
    double meanQ() const { return meanQ_; }
    double meanS() const { return meanS_; }

private:
    // Helpers to get samples; fall back to 1.0 if no callback attached
    double interArrival() { return inter_fn_ ? inter_fn_(inter_user_) : 1.0; }
    double serviceTime()  { return serv_fn_  ? serv_fn_ (serv_user_)  : 1.0; }

    void updateStats() {
        sumQ_ += (now_ - prev_) * queue_;
        sumS_ += (now_ - prev_) * system_;
        if (now_ > 0.0) {
            meanQ_ = sumQ_ / now_;
            meanS_ = sumS_ / now_;
        }
        if (obs_) obs_->onMean(now_, meanQ_, meanS_);
    }

    void arrival() {
        now_ = next_time_;
        double d = interArrival();
        double next_arrival = now_ + d;
        fel_.push(ARRIVAL, next_arrival);

        if (system_ == 0) {
            next_event_ = SERVICE; // start service immediately
            next_time_  = now_;
        } else {
            fel_.pop(next_event_, next_time_);
        }

        prev_ = now_;
        if (obs_) obs_->onPoint(now_, queue_);
        queue_++;
        system_++;
        if (obs_) obs_->onPoint(now_, queue_);
    }

    void service() {
        now_ = next_time_;
        double st = serviceTime();
        fel_.push(DEPARTURE, now_ + st);

        updateStats();
        prev_ = now_;

        if (obs_) obs_->onPoint(now_, queue_);
        if (queue_ > 0) queue_--;          // one leaves the queue to start service
        fel_.pop(next_event_, next_time_); // pull next event from FEL
        if (obs_) obs_->onPoint(now_, queue_);
    }

    void departure() {
        now_ = next_time_;
        updateStats();
        prev_ = now_;

        if (obs_) obs_->onPoint(now_, queue_);
        if (system_ > 0) system_--;        // one leaves the system
        if (obs_) obs_->onPoint(now_, queue_);
        if (obs_) obs_->onMean(now_, meanQ_, meanS_);

        if (queue_ > 0) {
            next_event_ = SERVICE;         // immediately start next service
            next_time_  = now_;
        } else {
            fel_.pop(next_event_, next_time_);
        }
    }

private:
    double end_time_;
    double now_, prev_;
    int    queue_, system_;
    double sumQ_, sumS_, meanQ_, meanS_;

    FEL    fel_;
    int    next_event_;   // next event id
    double next_time_;    // next event time

    ISimObserver* obs_;

    SampleFn inter_fn_;   void* inter_user_;
    SampleFn serv_fn_;    void* serv_user_;
};

} // namespace mm1
