#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <thread>

#include <csignal>

namespace skd {
using std::chrono::days;
using std::chrono::hours;
using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::time_point_cast;
using std::chrono::weeks;

enum class week_e { thursday = 0, friday, saturday, sunday, monday, tuesday, wednesday };

enum class period_e { second = 0, minute, hour, day, week };

struct day_time {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

namespace operators {
    constexpr day_time parse_day_time(const char* str_time, size_t size) {
        uint8_t nums[3] = {0, 0, 0};
        size_t num_i = 0;

        constexpr day_time error = {255, 255, 255};

        if (size > 8)
            return error;

        for (size_t i = 0; i < size; i += 3) {
            if (str_time[i] < '0' || str_time[i] > '9')
                return error;
            nums[num_i] = nums[num_i] * 10 + (str_time[i] - '0');

            if (i + 1 >= size)
                return error;
            if (str_time[i + 1] < '0' || str_time[i + 1] > '9')
                return error;
            nums[num_i] = nums[num_i] * 10 + (str_time[i + 1] - '0');

            if (i + 2 < size)
                if (str_time[i + 2] != ':')
                    return error;

            ++num_i;
        }

        day_time result = {0, 0, 0};
        if (num_i >= 1)
            result.hours = nums[0];
        if (num_i >= 2)
            result.minutes = nums[1];
        if (num_i >= 3)
            result.seconds = nums[2];

        return result;
    }

    template <typename = void>
    struct invalid_day_time_result {
        template <bool V = false>
        void operator()(day_time v) {
            static_assert(V, "Day time is invalid");
        }
    };

    template <typename C, C... Cs>
    constexpr auto operator"" _dtm() {
        constexpr char str[sizeof...(Cs)] = {Cs...};
        constexpr auto res = parse_day_time(str, sizeof...(Cs));
        if constexpr (res.hours < 24 && res.minutes < 60 && res.seconds < 60)
            return res;
        else
            return invalid_day_time_result<>()(res);
    }
} // namespace operators

template <typename ClockT = std::chrono::system_clock>
struct period {
    ClockT::time_point next_timepoint(const ClockT::time_point& now) const {
        switch (period) {
        case period_e::second: {
            auto now_seconds = time_point_cast<seconds>(now);
            auto next = now_seconds + second * multiplier;
            if (next <= now)
                next += seconds{1};
            return next;
        }
        case period_e::minute: {
            auto now_minutes = time_point_cast<minutes>(now);
            auto next = now_minutes + minute * multiplier + second;
            if (next <= now)
                next += minutes{1};
            return next;
        }
        case period_e::hour: {
            auto now_hours = time_point_cast<hours>(now);
            auto next = now_hours + hour * multiplier + minute + second;
            if (next <= now)
                next += hours{1};
            return next;
        }
        case period_e::day: {
            auto now_days = time_point_cast<days>(now);
            auto next = now_days + day * multiplier + hour + minute + second;
            if (next <= now)
                next += days{1};
            return next;
        }
        case period_e::week: {
            auto now_weeks = time_point_cast<weeks>(now);
            auto next = now_weeks + day + hour + minute + second;
            if (next <= now)
                next += weeks{1};
            return next;
        }
        }

        return {};
    }

    size_t multiplier = 1;
    period_e period;
    days day;
    hours hour;
    minutes minute;
    seconds second;
};

template <typename ClockT, typename T, period_e>
class task_setter_time;

template <typename ClockT, typename T>
class task_setter_time<ClockT, T, period_e(-1)> {
public:
    template <typename>
    friend class sked;

    task_setter_time(T* sked, size_t every = 1, bool ionce = false):
        sc(sked),
        period{
            .multiplier = every,
            .period = period_e::second,
            .day = days{0},
            .hour = hours{0},
            .minute = minutes{0},
            .second = seconds{0},
        },
        once(ionce) {}

    task_setter_time(const task_setter_time&) = delete;
    task_setter_time& operator=(const task_setter_time&) = delete;
    task_setter_time(task_setter_time&&) = delete;
    task_setter_time& operator=(task_setter_time&&) = delete;

    ~task_setter_time() {
        if (sc)
            sc->push_task(period, std::move(handler));
    }

    template <typename F>
    task_setter_time& operator()(F&& handler) {
        set_handler(std::forward<F>(handler));
        return *this;
    }

protected:
    template <typename F>
    void set_handler(F&& h) {
        handler = [f = std::forward<F>(h), once = once] {
            f();
            return once;
        };
    }

private:
    T* sc;

protected:
    skd::period<ClockT> period;

private:
    std::function<bool()> handler;
    bool once;
};

template <typename ClockT, typename T>
class task_setter_time<ClockT, T, period_e::second> : public task_setter_time<ClockT, T, period_e(-1)> {
public:
    using task_setter_time<ClockT, T, period_e(-1)>::task_setter_time;

    task_setter_time<ClockT, T, period_e(-1)>& at_second(size_t value = 0) {
        this->period.second = seconds{value};
        return *this;
    }
};

template <typename ClockT, typename T>
class task_setter_time<ClockT, T, period_e::minute> : public task_setter_time<ClockT, T, period_e::second> {
public:
    using task_setter_time<ClockT, T, period_e::second>::task_setter_time;

    task_setter_time<ClockT, T, period_e(-1)>& at_minute(size_t minute = 0, size_t at_second = 0) {
        this->period.second = seconds{at_second};
        this->period.minute = minutes{minute};
        return *this;
    }
};

template <typename ClockT, typename T>
class task_setter_time<ClockT, T, period_e::hour> : public task_setter_time<ClockT, T, period_e::minute> {
public:
    using task_setter_time<ClockT, T, period_e::minute>::task_setter_time;

    task_setter_time<ClockT, T, period_e(-1)>& at(size_t at_hour, size_t at_minute = 0, size_t at_second = 0) {
        if (int(this->period.period) < int(period_e::day))
            this->period.period = period_e::day;

        this->period.hour = hours{at_hour};
        this->period.minute = minutes{at_minute};
        this->period.second = seconds{at_second};
        return *this;
    }

    task_setter_time<ClockT, T, period_e(-1)>& at(day_time time) {
        return at(time.hours, time.minutes, time.seconds);
    }
};

template <typename ClockT, typename T>
class task_setter : task_setter_time<ClockT, T, period_e::hour> {
public:
    template <typename>
    friend class sked;

    using task_setter_time<ClockT, T, period_e::hour>::task_setter_time;

    static task_setter once() {
        return {nullptr, 1, true};
    }

    static task_setter every(size_t count = 1) {
        return {nullptr, count};
    }

    template <typename F>
    task_setter& second(F&& handler) {
        this->period.period = period_e::second;
        this->set_handler(std::forward<F>(handler));
        return *this;
    }

    task_setter_time<ClockT, T, period_e::second>& minute(size_t at_second = 0) {
        this->period.period = period_e::minute;
        this->period.second = seconds{at_second};
        return *this;
    }

    task_setter_time<ClockT, T, period_e::minute>& hour(size_t at_minute = 0, size_t at_second = 0) {
        this->period.period = period_e::hour;
        this->period.minute = minutes{at_minute};
        this->period.second = seconds{at_second};
        return *this;
    }

    task_setter_time<ClockT, T, period_e::hour>& day(day_time time) {
        return day(time.hours, time.minutes, time.seconds);
        return *this;
    }

    task_setter_time<ClockT, T, period_e::hour>& day(size_t at_hour = 0, size_t at_minute = 0, size_t at_second = 0) {
        this->period.period = period_e::day;
        this->period.hour = hours{at_hour};
        this->period.minute = minutes{at_minute};
        this->period.second = seconds{at_second};
        return *this;
    }

#define DEF_WEEKDAY(NAME)                                 \
    task_setter_time<ClockT, T, period_e::hour>& NAME() { \
        this->period.period = period_e::week;             \
        this->period.day = days{int(week_e::NAME)};       \
        return *this;                                     \
    }

    DEF_WEEKDAY(monday)
    DEF_WEEKDAY(tuesday)
    DEF_WEEKDAY(wednesday)
    DEF_WEEKDAY(thursday)
    DEF_WEEKDAY(friday)
    DEF_WEEKDAY(saturday)
    DEF_WEEKDAY(sunday)

#undef DEF_WEEKDAY
};

namespace details {
    class sked_sighandler {
    public:
        static sked_sighandler& instance() {
            static sked_sighandler inst;
            return inst;
        }

        static void handler(int signal) {
            // std::cerr << "Handle signal " << signal << std::endl;
            if (signal == SIGINT || signal == SIGTERM)
                instance().shutdown();
        }

        sked_sighandler() {
            setup_signals();
        }

        void shutdown() {
            {
                std::lock_guard lock{mtx};
                working = false;
            }
            cv.notify_all();
        }

        void setup_signals() {
            struct sigaction action = {nullptr};
            action.sa_handler = handler;
            sigaction(SIGINT, &action, nullptr);
            sigaction(SIGTERM, &action, nullptr);
        }

        void await_shutdown_signal() {
            std::unique_lock lock{mtx};
            cv.wait(lock, [this] { return working == false; });
        }

    private:
        bool working = true;
        std::condition_variable cv;
        std::mutex mtx;
    };
} // namespace details

struct await_for_shutdown_t {};

static inline constexpr auto await_for_shutdown = await_for_shutdown_t{};

template <typename ClockT = std::chrono::system_clock>
class sked {
public:
    using task = task_setter<ClockT, sked>;

    template <typename, typename>
    friend class task_setter;

    template <typename, typename, period_e>
    friend class task_setter_time;

    using id_t = uint64_t;
    static inline constexpr id_t no_id = std::numeric_limits<id_t>::max();

    struct task_handler {
        skd::period<ClockT> period;
        std::function<bool()> handler;
    };

    sked(): t(&sked::run, this) {}

    template <typename... Ts>
    sked(Ts&&... tasks) {
        (push_task_safe(tasks.period, std::move(tasks.handler)), ...);
        t = std::jthread(&sked::run, this);
    }

    template <typename... Ts>
    sked(await_for_shutdown_t, Ts&&... tasks): sked(std::forward<Ts>(tasks)...) {
        await_for_shutdown_dtor = true;
    }

    sked(const sked&) = delete;
    sked& operator=(const sked&) = delete;
    sked(sked&&) = delete;
    sked& operator=(sked&&) = delete;

    ~sked() {
        if (await_for_shutdown_dtor)
            await_for_shutdown();

        running.store(false, std::memory_order_relaxed);
        cv.notify_one();
    }

    task_setter<ClockT, sked> every(size_t count = 1) {
        return {this, count};
    }

    task_setter<ClockT, sked> once() {
        return {this, 1, true};
    }

    void await_for_shutdown() {
        details::sked_sighandler::instance().await_shutdown_signal();
        running.store(false, std::memory_order_relaxed);
        cv.notify_one();
    }

private:
    template <typename F>
    void push_task(const period<ClockT>& p, F&& task) {
        std::lock_guard lock{mtx};
        push_task_safe(p, std::forward<F>(task));
        cv.notify_one();
    }

    template <typename F>
    void push_task_safe(const period<ClockT>& p, F&& task) {
        auto now = ClockT::now();
        auto next_run = p.next_timepoint(now);
        auto id = next_id();

        tasks.emplace(id, task_handler{p, std::forward<F>(task)});
        task_ids.emplace(next_run, id);
    }

    void run() {
        std::vector<std::pair<typename ClockT::time_point, id_t>> reinsert_tasks;

        while (running.load(std::memory_order_relaxed)) {
            typename ClockT::time_point tp = tp.max();
            id_t id = no_id;

            /* Find nearest time point */
            {
                std::lock_guard lock{mtx};
                if (!task_ids.empty()) {
                    auto begin = task_ids.begin();
                    tp = begin->first;
                    id = begin->second;
                }
            }

            std::unique_lock lock{mtx};
            cv.wait_until(lock, tp);

            auto now = ClockT::now();

            if (id != no_id && now >= tp) {

                auto task_range = task_ids.equal_range(tp);
                for (auto it = task_range.first; it != task_range.second;) {
                    auto tp = it->first;
                    auto id = it->second;

                    /* Launch task */
                    bool delete_task = true;
                    auto task_p = tasks.find(id);
                    if (task_p != tasks.end())
                        delete_task = task_p->second.handler();

                    task_ids.erase(it++);

                    if (delete_task) {
                        tasks.erase(id);
                    }
                    else {
                        auto next = task_p->second.period.next_timepoint(now);
                        reinsert_tasks.emplace_back(next, id);
                    }
                }

                for (auto&& [next, id] : reinsert_tasks)
                    task_ids.emplace(next, id);

                reinsert_tasks.clear();
            }
        }
    }

    id_t next_id() {
        return id_counter++;
    }

private:
    std::jthread t;
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::atomic_bool running = true;
    bool await_for_shutdown_dtor = false;

    std::map<id_t, task_handler> tasks;
    std::multimap<typename ClockT::time_point, id_t> task_ids;
    id_t id_counter = 0;
};
} // namespace skd
