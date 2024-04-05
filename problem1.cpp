#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <condition_variable>
#include <random>
#include <atomic>

std::mutex output_mutex;

void safe_print(const std::string &message)
{
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cout << message << std::endl;
}

class ConcurrentLinkedList
{
private:
    std::list<int> list;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> presentCount{0}, thankYouNoteCount{0};
    std::atomic<bool> doneAdding{false};
    int maxPresents;

public:
    ConcurrentLinkedList(int max) : maxPresents(max) {}

    bool add(int tag)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (presentCount >= maxPresents || doneAdding)
        {
            return false; // Prevent adding if max reached or done adding
        }
        auto it = std::lower_bound(list.begin(), list.end(), tag);
        list.insert(it, tag);
        presentCount++;
        cv.notify_one(); // Notify potentially waiting removers
        return true;
    }

    bool remove()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]
                { return !list.empty() || doneAdding; }); // Wait until there's something to remove or we're done adding
        if (!list.empty())
        {
            list.pop_front();
            thankYouNoteCount++;
            return true;
        }
        return false;
    }

    void finishAdding()
    {
        doneAdding = true;
        cv.notify_all(); // Notify all waiting threads
    }

    bool isWorkDone() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return doneAdding && list.empty(); // Check if we're done adding and list is empty
    }

    bool isAddingDone() const
    {
        return doneAdding.load();
    }

    std::pair<int, int> getTaskCounts() const
    {
        return {presentCount.load(), thankYouNoteCount.load()};
    }

    void waitForAllNotes()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]
                { return list.empty(); }); // Wait until all notes are written
    }
};

void servantTask(ConcurrentLinkedList &chain, int id, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, max - 1);

    while (!chain.isWorkDone())
    { // Keep working until done
        int action = dis(gen) % 3;
        if (action == 0 && !chain.isAddingDone())
        {
            int tag = dis(gen);
            if (chain.add(tag))
            {
                safe_print("Servant " + std::to_string(id) + " added present " + std::to_string(tag));
            }
        }
        else if (action == 1)
        {
            if (chain.remove())
            {
                safe_print("Servant " + std::to_string(id) + " wrote a 'Thank you' card");
            }
        }
        else
        {
            int tag = dis(gen);
            if (chain.search(tag))
            {
                safe_print("Servant " + std::to_string(id) + " confirmed present " + std::to_string(tag) + " is in the chain");
            }
            else
            {
                safe_print("Servant " + std::to_string(id) + " did not find present " + std::to_string(tag) + " in the chain");
            }
        }
    }
}

int main()
{
    const int numPresents = 100;
    const int numServants = 4;
    ConcurrentLinkedList chain(numPresents);
    std::vector<std::thread> servants;

    for (int i = 0; i < numServants; ++i)
    {
        servants.emplace_back(servantTask, std::ref(chain), i + 1, numPresents);
    }

    for (auto &servant : servants)
    {
        servant.join();
    }

    // Ensure all "Thank You" notes are written
    chain.waitForAllNotes();

    auto [presentsAdded, thankYouNotesWritten] = chain.getTaskCounts();
    safe_print("All tasks complete. Presents added: " + std::to_string(presentsAdded) + ", Thank You Notes written: " + std::to_string(thankYouNotesWritten));

    return 0;
}
