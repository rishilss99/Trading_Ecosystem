#include <iostream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <variant>
#include <tuple>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

inline auto ASSERT(bool cond, const std::string &msg) noexcept
{
    if (UNLIKELY(!cond))
    {
        std::cerr << msg << std::endl;
        exit(EXIT_FAILURE);
    }
}

inline auto FATAL(const std::string &msg) noexcept
{
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

inline auto setThreadCore(int core_id) noexcept
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0);
}

template <typename T, typename... A>
inline auto createAndStartThread(int core_id, const std::string &name, T &&func, A... args)
{
    auto t = new std::thread([&]()
                             {       
        if (core_id >= 0 && !setThreadCore(core_id)) {
        std::cerr << "Failed to set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;
        exit(EXIT_FAILURE);
        } 
      std::cerr << "Set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;

      std::forward<T>(func)((std::forward<A>(args))...); }); // THIS LINE IS VERY IMPORTANT AND WORTH UNDERSTANDING

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
    return t;
}

template <typename T>
decltype(auto) move(T &&param)
{
    using ReturnType = std::remove_reference_t<T> &&;
    return static_cast<ReturnType>(param);
}

auto dummyAdditionFunction(int a, int b, bool sleep)
{
    std::cout << "Dummy addition for thread " << std::this_thread::get_id() << " with numbers " << a << " and " << b << "\n";
    std::cout << "The output is " << a + b << "\n";
    if (sleep)
    {
        std::cout << "Thread " << std::this_thread::get_id() << " going to sleep\n";
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(2s);
    }
    std::cout << "Thread " << std::this_thread::get_id() << " is done\n";
}

template <typename T>
T sum(T x)
{
    return x;
}

template <typename T, typename... Args>
T sum(T x, Args... args)
{
    return x + sum(args...);
}

template <typename T>
class Mempool final
{
public:
    Mempool() = delete;
    Mempool(const Mempool &) = delete;
    Mempool(Mempool &&) = delete;
    Mempool &operator=(const Mempool &) = delete;
    Mempool &operator=(Mempool &&) = delete;
    explicit Mempool(std::size_t num_elems) : store_(num_elems, {T(), true})
    {
        ASSERT(reinterpret_cast<const ObjectBlock *>(&(store_[0].object_)) == &(store_[0]), "T object should be first member of ObjectBlock.");
    }

    template <typename... Args>
    T *allocate(Args... args) noexcept
    {
        auto obj_block = &(store_[next_free_idx_]);
        ASSERT(obj_block->is_free_, "Expected free ObjectBlock at index:" + std::to_string(next_free_idx_));
        T *ret = &(obj_block->object_);
        ret = new (ret) T(args...);
        obj_block->is_free_ = false;
        updateNextFreeIndex();
        return ret;
    }

    auto deallocate(const T *elem) noexcept
    {
        const auto elem_index = reinterpret_cast<const ObjectBlock *>(elem) - &(store_[0]);
        ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index < store_.size()), "Element being deallocated does not belong to this Memory pool.");
        ASSERT(!store_[elem_index].is_free_, "Expected in-use ObjectBlock at index:" + std::to_string(elem_index));
        store_[elem_index].is_free_ = true;
    }

private:
    auto updateNextFreeIndex() noexcept
    {
        const auto inital_free_index = next_free_idx_;
        while (!store_[next_free_idx_].is_free_)
        {
            ++next_free_idx_;
            if (UNLIKELY(next_free_idx_ == store_.size()))
            {
                next_free_idx_ = 0;
            }
            if (UNLIKELY(inital_free_index == next_free_idx_))
            {
                ASSERT(inital_free_index != next_free_idx_, "Memory Pool out of space.");
            }
        }
    }

    struct ObjectBlock
    {
        T object_;
        bool is_free_ = true;
    };
    std::vector<ObjectBlock> store_;
    std::size_t next_free_idx_ = 0;
};

template <typename T>
class Type;

struct MyStruct
{
    int d_[3];
};

void MempoolExample()
{
    Mempool<double> double_pool(10);
    Mempool<MyStruct> my_struct_pool(10);

    // Separating out the allocation from initilization
    // MyStruct *m = new(MyStruct);
    // m = new(m) MyStruct{1,2,3};

    for (int i = 0; i < 10; i++)
    {
        double *double_ptr = double_pool.allocate(i);
        MyStruct *my_struct_ptr = my_struct_pool.allocate(MyStruct{i, i + 1, i + 2});

        std::cout << "Double elem:" << *double_ptr << " allocated at:"
                  << double_ptr << std::endl;
        std::cout << "Struct elem:" << my_struct_ptr->d_[0] << "," << my_struct_ptr->d_[1] << "," << my_struct_ptr->d_[2] << " allocated at : " << my_struct_ptr << std::endl;
        if (i % 5 == 0)
        {
            std::cout << "deallocating double elem:" << *double_ptr << "from : " << double_ptr << std::endl;
            std::cout << "deallocating struct elem:" << my_struct_ptr->d_[0] << "," << my_struct_ptr->d_[1] << "," << my_struct_ptr->d_[2] << " from:" << my_struct_ptr << std::endl;
            double_pool.deallocate(double_ptr);
            my_struct_pool.deallocate(my_struct_ptr);
        }
    }
}

template <typename T>
class LFQueue final
{
public:
    LFQueue() = delete;
    LFQueue(const LFQueue &) = delete;
    LFQueue(LFQueue &&) = delete;
    LFQueue &operator=(const LFQueue &) = delete;
    LFQueue &operator=(LFQueue &&) = delete;
    explicit LFQueue(size_t num_elements) : store_(num_elements, T{}) {}

    auto getNextToWriteTo() noexcept
    {
        return &store_[next_write_idx_.load()];
    }

    auto updateWriteIndex() noexcept
    {
        size_t curr_idx = next_write_idx_.load();
        size_t new_idx;
        do
        {
            new_idx = (curr_idx + 1) % store_.size();
        } while (!next_write_idx_.compare_exchange_strong(curr_idx, new_idx));
        num_elements_++;
    }

    auto getNextToReadFrom() noexcept
    {
        return (next_read_idx_.load() == next_write_idx_.load()) ? nullptr : &store_[next_read_idx_.load()];
    }

    auto updateReadIndex() noexcept
    {
        size_t curr_idx = next_read_idx_.load();
        size_t new_idx;
        do
        {
            new_idx = (curr_idx + 1) % store_.size();
        } while (!next_read_idx_.compare_exchange_strong(curr_idx, new_idx));
        ASSERT(num_elements_ != 0, "Read an invalid element in:" + std::to_string(pthread_self()));
        num_elements_--;
    }

    auto size() const noexcept
    {
        return num_elements_.load();
    }

private:
    std::vector<T> store_;
    std::atomic<size_t> next_read_idx_ = 0;
    std::atomic<size_t> next_write_idx_ = 0;
    std::atomic<size_t> num_elements_ = 0;
};

void LFQueueExample()
{
    auto consumerFunction = [](LFQueue<MyStruct> *lfq)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);
        while (lfq->size())
        {
            const auto data = lfq->getNextToReadFrom();
            lfq->updateReadIndex();
            std::cout << "consumeFunction read elem:" << data->d_[0]
                      << "," << data->d_[1] << "," << data->d_[2] << " lfq-size:"
                      << lfq->size() << std::endl;
            std::this_thread::sleep_for(1s);
        }
        std::cout << "consumerFunction exiting." << std::endl;
    };

    LFQueue<MyStruct> lfq(20);
    auto ct = createAndStartThread(-1, "", consumerFunction, &lfq);

    for (auto i = 0; i < 10; i++)
    {
        const MyStruct data{i, i * 10, i * 100};
        *(lfq.getNextToWriteTo()) = data;
        lfq.updateWriteIndex();
        std::cout << "main constructed elem:" << data.d_[0] << ","
                  << data.d_[1] << "," << data.d_[2] << " lfq-size:" << lfq.size() << std::endl;
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    ct->join();
    std::cout << "main exiting." << std::endl;
}

struct Im
{
    Im() { ; }
    Im(int) { ; }
    Im(int, int) { ; }
};

void readIm(const Im &) { ; }

struct ListNode
{
    int val;
    ListNode *next;
};

class LockFreeList
{
public:
    void push_front(int x)
    {
        ListNode *new_node = new ListNode();
        new_node->val = x;
        ListNode *old_head = head;
        do
        {
            new_node->next = old_head;
        } while (!head.compare_exchange_strong(old_head, new_node));
    }

private:
    std::atomic<ListNode *> head;
};

namespace logger
{
    using Nanos = int64_t;
    constexpr Nanos NANOS_TO_MICROS = 1000;
    constexpr Nanos MICROS_TO_MILLIS = 1000;
    constexpr Nanos MILLIS_TO_SECS = 1000;
    constexpr Nanos NANOS_TO_MILLIS = NANOS_TO_MICROS * MICROS_TO_MILLIS;
    constexpr Nanos NANOS_TO_SECS = NANOS_TO_MILLIS * MILLIS_TO_SECS;

    inline auto getCurrentNanos() noexcept
    {
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    inline auto &getCurrentTimeStr(std::string *time_str)
    {
        const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        time_str->assign(ctime(&time));
        if (!time_str->empty())
        {
            time_str->at(time_str->length() - 1) = '\0';
        }
        return *time_str;
    }

    constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;
    enum class LogType : int8_t
    {
        CHAR = 0,
        INTEGER = 1,
        LONG_INTEGER = 2,
        LONG_LONG_INTEGER = 3,
        UNSIGNED_INTEGER = 4,
        UNSIGNED_LONG_INTEGER = 5,
        UNSIGNED_LONG_LONG_INTEGER = 6,
        FLOAT = 7,
        DOUBLE = 8
    };

    using MyTypes = std::tuple<char, int, long, long long, unsigned, unsigned long, unsigned long long, float, double>;

    template <typename T, typename Tuple>
    struct TypeIndex;

    template <typename T, typename... Types>
    struct TypeIndex<T, std::tuple<T, Types...>>
    {
        static constexpr std::size_t value = 0;
    };

    template <typename T, typename U, typename... Types>
    struct TypeIndex<T, std::tuple<U, Types...>>
    {
        static constexpr std::size_t value = 1 + TypeIndex<T, std::tuple<Types...>>::value;
    };

    template <typename T>
    constexpr std::size_t getIndex()
    {
        return TypeIndex<T, MyTypes>::value;
    }

    template <std::size_t Index, typename Tuple>
    using MyType = typename std::tuple_element<Index, Tuple>::type;

    template <typename... Types>
    struct TupleToVariant;

    template <typename... Types>
    struct TupleToVariant<std::tuple<Types...>>
    {
        using type = std::variant<Types...>;
    };

    struct LogElement
    {
        LogType type_ = LogType::CHAR;
        union
        {
            char c;
            int i;
            long l;
            long ll;
            unsigned int u;
            unsigned long ul;
            unsigned long long ull;
            float f;
            double d;
        } u_;
        // using MyVariant = TupleToVariant<MyTypes>::type;
        // MyVariant v_;
    };

    class Logger final
    {
    public:
        Logger() = delete;
        Logger(const Logger &) = delete;
        Logger(Logger &&) = delete;
        Logger &operator=(const Logger &) = delete;
        Logger &operator=(Logger &&) = delete;
        auto flushQueue() noexcept
        {
            while (running_.load())
            {
                for (auto next = queue_.getNextToReadFrom(); queue_.size() && next; next = queue_.getNextToReadFrom())
                {
                    // std::get<MyType<static_cast<size_t>(next->type_),MyTypes>>(next->v_);
                    switch (next->type_)
                    {
                    case LogType::CHAR:
                        file_ << next->u_.c;
                        break;
                    case LogType::INTEGER:
                        file_ << next->u_.i;
                        break;
                    case LogType::LONG_INTEGER:
                        file_ << next->u_.l;
                        break;
                    case LogType::LONG_LONG_INTEGER:
                        file_ << next->u_.ll;
                        break;
                    case LogType::UNSIGNED_INTEGER:
                        file_ << next->u_.u;
                        break;
                    case LogType::UNSIGNED_LONG_INTEGER:
                        file_ << next->u_.ul;
                        break;
                    case LogType::UNSIGNED_LONG_LONG_INTEGER:
                        file_ << next->u_.ull;
                        break;
                    case LogType::FLOAT:
                        file_ << next->u_.f;
                        break;
                    case LogType::DOUBLE:
                        file_ << next->u_.d;
                        break;
                    }
                    queue_.updateReadIndex(); // The only reason this works is since we have a SPSC model
                }
                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }
        }

        explicit Logger(const std::string &file_name) : name_(file_name), queue_(LOG_QUEUE_SIZE)
        {
            file_.open(file_name);
            ASSERT(file_.is_open(), "Could not open log file:" + file_name);
            logger_thread_ = createAndStartThread(-1, "Common/Logger", [this]()
                                                  { flushQueue(); });
            // using namespace std::literals::chrono_literals;
            // std::this_thread::sleep_for(1ms);
            ASSERT(logger_thread_ != nullptr, "Failed to start Logger thread");
        }

        auto pushValue(const LogElement &log_element) noexcept
        {
            *(queue_.getNextToWriteTo()) = log_element;
            queue_.updateWriteIndex();
        }

        auto pushValue(const char value) noexcept
        {
            pushValue(LogElement{LogType::CHAR, {.c = value}});
        }

        auto pushValue(const char *value) noexcept
        {
            while (*value)
            {
                pushValue(*value);
                value++;
            }
        }

        auto pushValue(const std::string &value) noexcept
        {
            pushValue(value.c_str());
        }

        // template <typename T>
        // auto pushValue(const T value)
        // {
        //     pushValue(LogElement{static_cast<LogType>(getIndex<T>()), value});
        // }

        auto pushValue(const int value) noexcept
        {
            pushValue(LogElement{LogType::INTEGER, {.i = value}});
        }

        auto pushValue(const long value) noexcept
        {
            pushValue(LogElement{LogType::LONG_INTEGER, {.l = value}});
        }

        auto pushValue(const long long value) noexcept
        {
            pushValue(LogElement{LogType::LONG_LONG_INTEGER, {.ll = value}});
        }

        auto pushValue(const unsigned value) noexcept
        {
            pushValue(LogElement{LogType::UNSIGNED_INTEGER, {.u = value}});
        }

        auto pushValue(const unsigned long value) noexcept
        {
            pushValue(LogElement{LogType::UNSIGNED_LONG_INTEGER, {.ul = value}});
        }

        auto pushValue(const unsigned long long value) noexcept
        {
            pushValue(LogElement{LogType::UNSIGNED_LONG_LONG_INTEGER, {.ull = value}});
        }

        auto pushValue(const float value) noexcept
        {
            pushValue(LogElement{LogType::FLOAT, {.f = value}});
        }

        auto pushValue(const double value) noexcept
        {
            pushValue(LogElement{LogType::DOUBLE, {.d = value}});
        }

        template <typename T, typename... Args>
        auto log(const char *s, const T &value, Args... args) noexcept
        {
            while (*s)
            {
                if (*s == '%')
                {
                    if (UNLIKELY(*(s + 1) == '%'))
                    {
                        ++s;
                    }
                    else
                    {
                        pushValue(value);
                        log(s + 1, args...);
                        return;
                    }
                }
                else
                {
                    pushValue(*s++);
                }
            }
            FATAL("extra arguments provided to log()");
        }

        auto log(const char *s)
        {
            while (*s)
            {
                if (*s == '%')
                {
                    if (UNLIKELY(*(s + 1) == '%'))
                    {
                        ++s;
                    }
                    else
                    {
                        FATAL("missing arguments to log()");
                    }
                }
                pushValue(*s++);
            }
        }

        ~Logger()
        {
            std::cerr << "Flushing and closing Logger for " << name_ << std::endl;
            while (queue_.size())
            {
                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(1s);
            }
            running_.store(false);
            logger_thread_->join();
            file_.close();
        }

    private:
        std::string name_;
        std::ofstream file_;
        LFQueue<LogElement> queue_;
        std::atomic<bool> running_ = {true};
        std::thread *logger_thread_ = nullptr;
    };
}

int main()
{
    // auto t1 = createAndStartThread(1, "Thread 1", dummyAdditionFunction, 5, 6, false);
    // // auto t2 = createAndStartThread(-1, "Thread 2", dummyAdditionFunction, 10, 11, true);
    // t1->join();
    // // t2->join();
    // std::cout << "All done\n";
    // std::cout << "Sum: " << sum(5, 3, 4, 5, 1, 10) << "\n";

    // Argument for explicit
    readIm({5});
    readIm({4, 5});
    // MempoolExample();

    // LFQueue
    // LFQueueExample();

    // Logger Example
    std::string a;
    std::string b = logger::getCurrentTimeStr(&a);
    using namespace logger;
    Logger logging("logging_example.log");
    char k = 'g';
    int q = 6;
    logging.log("Logging a char: %, int: %\n", k, q);
    std::string s = "Test String";
    logging.log("Logging a string: %\n", s);
    float f = 3.45;
    logging.log("Logging a float: %\n", f);
    const char st[] = "Hello";
    logging.log("Logging a character array: '%'\n", st);
}