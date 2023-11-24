#include <thread>
#include <iostream>
#include <semaphore>
#include <queue>
#include <random>
#include <chrono>

using namespace std;

//The buffer
queue<int> buffer;

//Semaphores

int randInt(int lowerBound, int upperBound) {
    // Seed the random number generator
    random_device rd;
    mt19937 gen(rd());

    //Define the range for the random number
    uniform_int_distribution<> distribution(lowerBound, upperBound);

    // Output the random number
    return distribution(gen);
}


void consumerFunction() {
    // Wait 10 seconds for empty slot
    bool queue_empty = false;
    while (queue_empty) {
        // if 10_seconds_passed
            // kill thread
    }
    cout << "Consumer thread with id " << this_thread::get_id();
    cout << " accesses the buffer\n";
    int queueFront = buffer.front(); // get first job from buffer
    buffer.pop(); // delete job from the buffer
    // Sleep for the duration of time specified in the buffer
    auto sleepTime = std::chrono::seconds(queueFront);
    cout << "Consumer thread with id " << this_thread::get_id();
    cout << " sleeps for " << queueFront << " seconds\n";
    this_thread::sleep_for(sleepTime);
}

counting_semaphore<> space;
counting_semaphore<> item(0);
binary_semaphore bufferMutex;

void producerFunction() {
    bool queue_full = false;
    while (queue_full) {
        // if 10_seconds_passed
            // kill thread
    }
    // Produce item
    int newNumber = (randInt(1, 10));
    
    // down(space): check if space left in queue
    // down(mutex): test and set the lock

    cout << "Consumer thread with id " << this_thread::get_id();
    cout << " accesses the buffer\n";

    buffer.push(newNumber);

    cout << "Producer thread with id " << this_thread::get_id();
    cout << " adds " << newNumber << " to the front of the buffer\n";
}


int main(const int queueSize, int noJobs, int noProducers, int noConsumers) {
    // Initialise semaphores
    // mutex bufferMutex;
    static counting_semaphore<> space(queueSize);
    static counting_semaphore<> item(0);

    thread producer(producerFunction);
    thread consumer(consumerFunction);
}
