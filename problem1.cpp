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
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> presents{0}, notes{0};
    int maxPresents;

public:
    ConcurrentLinkedList(int max) : maxPresents(max) {}

    bool add(int id)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (presents >= maxPresents)
            return false;

        int tag = presents++;
        list.push_back(tag);
        safe_print("Servant " + std::to_string(id) + " added present " + std::to_string(tag + 1));
        cv.notify_all();
        return true;
    }

    bool remove(int id)
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]
                { return !list.empty() || presents >= maxPresents; });

        if (!list.empty())
        {
            int tag = list.front();
            list.pop_front();
            notes++;
            safe_print("Servant " + std::to_string(id) + " wrote a 'Thank you' note for present " + std::to_string(tag + 1));
            cv.notify_all();
            return true;
        }
        return false;
    }

    bool search(int id, int tag)
    {
        std::unique_lock<std::mutex> lock(mutex);
        bool found = std::find(list.begin(), list.end(), tag) != list.end();
        safe_print("Servant " + std::to_string(id) + (found ? " confirmed present " : " did not find present ") + std::to_string(tag + 1));
        return found;
    }

    int getPresentsCount() const { return presents; }
    int getNotesCount() const { return notes; }
};

void servantTask(ConcurrentLinkedList &chain, int id, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, max - 1);

    while (chain.getNotesCount() < max)
    {
        int action = dis(gen) % 3;
        if (action == 0)
        {
            chain.add(id);
        }
        else if (action == 1)
        {
            chain.remove(id);
        }
        else
        {
            int search_tag = dis(gen) % max;
            chain.search(id, search_tag);
        }
    }
}

int main()
{
    const int numPresents = 100;
    ConcurrentLinkedList chain(numPresents);

    std::vector<std::thread> servants;
    for (int i = 0; i < 4; ++i)
    {
        servants.emplace_back(servantTask, std::ref(chain), i + 1, numPresents);
    }

    for (auto &servant : servants)
    {
        servant.join();
    }

    safe_print("All tasks complete. Presents added: " + std::to_string(chain.getPresentsCount()) + ", Thank You Notes written: " + std::to_string(chain.getNotesCount()));

    return 0;
}
