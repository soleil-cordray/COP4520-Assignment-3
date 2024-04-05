#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>          // For std::find_if and std::find
#include <condition_variable> // For std::condition_variable
#include <random>
#include <atomic>

// 500,000 presents in unordered bag
// each present has tag w/ unique guest (gifter) num
// create chain of presents hooked together via linked list
// presents ordered according to increasing tag num
// 4 servants (threads) write cards in order of this increasing num (implement concurrent linked list)
// take present from bag, add to chain in correct location/order (hook to predecessor link). make sure it's linked to next present in chain
// write "thank you" card by unlinking gift from predecessor & connecting predecessor link to next gift
// check whether gift w/ tag present (scan w/o adding/removing a new gift). check whther gift w/ particular tag already added or not
// don't wait until all presents linked
// servants (threads) alternate adding gifts to ordered chain & writing thank yous
// don't stop/take break until writing all cards complete
// end: more presents than "thank you" notes.' IMPROVE STRATREGY SO THIS DOESN'T HAPPEN.

std::mutex output_mutex;
std::mutex count_mutex;

void safe_print(const std::string &message)
{
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cout << message << std::endl;
}

class ConcurrentLinkedList
{
public:
    // add present tag sorted into list (requires waiting))
    bool add(int tag, std::atomic<int> &count, int max)
    {
        std::unique_lock<std::mutex> lock(mutex);

        // wait until right turn to add present
        cv.wait(lock, [&]
                { return tag == count.load(std::memory_order_relaxed) || count.load(std::memory_order_relaxed) >= max; });

        // proceed only if right turn & max not reached
        if (tag == count.load(std::memory_order_relaxed) && tag < max)
        {
            list.push_back(tag); // assume list sorted
            count.fetch_add(1, std::memory_order_relaxed);
            cv.notify_all(); // notify others turn
            return true;
        }

        return false;
    }

    // remove present tag from sorted list
    bool remove(int &tag)
    {
        std::unique_lock<std::mutex> lock(mutex);

        // wait until list not empty
        cv.wait(lock, [this]
                { return !list.empty(); });

        // get (smallest) tag from front & remove
        tag = list.front();
        list.pop_front();

        return true;
    }

    // search present tag in sorted list
    bool search(int tag)
    {
        // only lock (without waiting) within scope
        std::lock_guard<std::mutex> lock(mutex);

        // look for tag till end of list; return true if found, false if not
        return std::find(list.begin(), list.end(), tag) != list.end();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return list.size();
    }

private:
    std::list<int> list;
    std::mutex mutex;
    std::condition_variable cv;
};

void servantTask(ConcurrentLinkedList &list, int max, int id, std::atomic<int> &count)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, max - 1);

    int action = id % 3; // Start each servant on a different action for diversity

    // until all presents processed
    while (count.load(std::memory_order_relaxed) < max)
    {
        if (action == 0) // action 1: add
        {
            int tag = count.load(std::memory_order_relaxed);
            if (tag < max && list.add(tag, count, max)) // if within bounds
            {
                safe_print("Servant " + std::to_string(id) + " added present " + std::to_string(tag + 1));
            }
        }
        else if (action == 1) // action 2: remove 
        {
            int tag;
            if ((list.size() > 0) && list.remove(tag))
            {
                safe_print("Servant " + std::to_string(id) + " wrote a 'Thank You' card for present " + std::to_string(tag + 1));
            }
        }
        else if (action == 2) // action 3: search
        {
            // generate tag within range to check
            int search_tag = dis(gen);

            if (list.search(search_tag))
            {
                safe_print("Servant " + std::to_string(id) + " checked and found present " + std::to_string(search_tag + 1));
            }
            else
            {
                safe_print("Servant " + std::to_string(id) + " checked and did not find present " + std::to_string(search_tag + 1));
            }
        }

        // move next action (round-robin fasion)
        action = (action + 1) % 3;
    }
}

int main()
{
    const int servants = 4;
    const int presents = 100;
    ConcurrentLinkedList list;
    std::vector<std::thread> threads;
    std::atomic<int> count(0);

    // create & start unique servant threads
    for (int i = 0; i < servants; ++i)
    {
        threads.emplace_back(servantTask, std::ref(list), presents, i + 1, std::ref(count));
    }

    // join to ensure execution completion
    for (auto &thread : threads)
    {
        thread.join();
    }

    return 0;
}
