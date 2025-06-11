/**
 * @file CommandFifo.h
 * @brief CommandFifo class for sending commands to the audio thread.
 * @author JUCE examples (see the Sampler)
 */

 #pragma once

// CommandFifo.h
// We want to send type-erased commands to the audio thread, but we also
// want those commands to contain move-only resources, so that we can
// construct resources on the gui thread, and then transfer ownership
// cheaply to the audio thread. We can't do this with std::function
// because it enforces that functions are copy-constructible.
// Therefore, we use a very simple templated type-eraser here.
template <typename Proc>
struct Command {
    virtual ~Command() noexcept = default;
    virtual void run(Proc& proc) = 0;
};

template <typename Proc, typename Func>
class TemplateCommand : public Command<Proc>, private Func {
public:
    template <typename FuncPrime>
    explicit TemplateCommand(FuncPrime&& funcPrime) : Func(std::forward<FuncPrime>(funcPrime)) {
    }
    void run(Proc& proc) override { (*this) (proc); }
};

template <typename Proc>
class CommandFifo final {
public:
    explicit CommandFifo(int size) : buffer((size_t)size), abstractFifo(size) {
    }

    CommandFifo() : CommandFifo(1024) {
    }

    template <typename Item>
    void push(Item&& item) noexcept {
        auto command = makeCommand(std::forward<Item>(item));

        abstractFifo.write(1).forEach([&](int index)
            {
                buffer[size_t(index)] = std::move(command);
            });
    }

    void call(Proc& proc) {
        abstractFifo.read(abstractFifo.getNumReady()).forEach([&](int index) {
            buffer[size_t(index)]->run(proc);
        });
    }

    void reset() {
        abstractFifo.reset();
    }
private:
    template <typename Func>
    static std::unique_ptr<Command<Proc>> makeCommand(Func&& func) {
        using Decayed = typename std::decay<Func>::type;
        return std::make_unique<TemplateCommand<Proc, Decayed>>(std::forward<Func>(func));
    }

    std::vector<std::unique_ptr<Command<Proc>>> buffer;
    juce::AbstractFifo abstractFifo;
};


/**
 * @brief A simple string list that allocates strings from a fixed-size heap.
 */
template <int itemSize, int heapSize>
class HeapStringList {
    char *items[itemSize];
    char buffer[heapSize];
    int heapPos = 0;
    int numItems = 0;
public:
    HeapStringList() {
        buffer[0] = '\0';
    }

    void add(const char *str) {
        if (numItems >= itemSize)
            return;

        int len = strlen(str);
        if (len+1+heapPos >= heapSize)
            return;

        items[numItems] = buffer + heapPos;
        memcpy(buffer + heapPos, str, len);
        buffer[heapPos + len] = '\0';
        heapPos += len + 1;
        numItems++;
    }

    int size() const {
        return numItems;
    }

    char *getItem(int idx) {
        if (idx < 0 || idx >= numItems) {
            return buffer + heapPos;
        }
        return items[idx];
    }
};

/**
 * @brief Helper class to wait for a reply from the audio thread.
 * The goal is to allocate the reply object from the UI thread and let the audio thread fill it.
 *
 * This could be done with a std::promise, but we want to avoid the overhead of std::future that would 
 * allocte from the audio thread.
 */
template <class T>
class ASyncReply {
    const int TIMEOUT_SECONDS = 1;
    int rc = -1;

    std::condition_variable cv;
    std::mutex cv_m;
    std::unique_lock<std::mutex> lk;
public:
    T content;

    ASyncReply() : lk(cv_m) {
    }

    /**
     * Waits for the audio thread to notify this object.
     * If the audio thread does not notify within TIMEOUT_SECONDS, it returns -1.
     */
    int wait() {
        cv.wait_for(lk, std::chrono::seconds(TIMEOUT_SECONDS));
        return rc;
    }

    void notify(int rc) {
        this->rc = rc;
        cv.notify_one();
    }
};
