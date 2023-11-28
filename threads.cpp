#include <thread>
#include <iostream>
#include <queue>
#include <sstream>
#include <semaphore>

using namespace std;

int queueSize;
int noJobs;
int noProducers;
int noConsumers;

class CircularQueue {
private:
  int rear, front, size;
  int* items;
public:
  CircularQueue(int inputSize): size(inputSize) {
      front = -1; // invalid default value
      rear = -1; // invalid default value
      try {
	items = new int[size];
      } catch (const bad_alloc& e) {
	cerr << "Error: " << e.what() << '\n';
	exit(1);
      }
      for (int i = 0; i < size; i++)
	items[i] = -1;
    }
 
  void enQueue(int value) {
    if (front == 0 && rear == size-1) {
      cerr << "Error: tried to enQueue into full queue";
      exit(1);
    }
    else if (front == -1) { // Array is empty
      front = 0;
      rear = 0; // = front of array
      items[rear] = value;
    }
    else if (rear == size-1 && front != 0) {
      rear = 0; // wrap
      items[rear] = value;
    }
    else {
      rear++;
      items[rear] = value;
    }
  }
  int deQueue() {
    if (front == -1) {
      cerr << "Error: tried to deQueue from empty queue \n";
      exit(1);
    }
    int front_value = items[front];
    items[front] = -1; // invalid value
    if (front == rear) {
      front = -1; // invalid value
      rear = -1; // invalid value
    }
    else {
      front++;
    }
    if (front == size)
      front = 0; // wrap
    return front_value;
  }

  void queueToSstream(stringstream& outstream) {
    outstream << "Buffer is: {";
    int i = 0;
    do {
	if (i == front)
	  outstream << "front: ";
	if (i == rear)
	  outstream << "rear: ";
	outstream << items[i] << ", ";
    } while (++i < size-1);
    if (i == front)
       outstream << "front: ";
    if (i == rear)
       outstream << "rear: ";
    outstream << items[size-1] << "}\n";
  }

  void clear() {
    delete[] items;
  }
};

/* Global mutex for buffer access */
binary_semaphore bufferMutex(1);

void consumer(CircularQueue& buffer, counting_semaphore<1000>& item, counting_semaphore<1000>& space) {
    /* Announce thread creation */
    stringstream outstream;
    outstream << "Consumer thread with id "  << this_thread::get_id() << " created\n";
    cout << outstream.str();
    outstream.str("");
    if (!item.try_acquire_for(10s)) {
        /* Timeout */
        outstream << "Consumer thread with id " << this_thread::get_id();
        outstream << " times out\n";
        cout << outstream.str();
        return;
    }
    bufferMutex.acquire();
    int queueFront = buffer.deQueue(); // pop first job from the buffer

    /* Announce successful consumer entry to the buffer. This is done before the
        semaphores are released since it needs to read from the critical
        section in order to write the buffer contents to the console
    */
    outstream << "Consumer thread with id " << this_thread::get_id()
    << " takes " << queueFront << " from the front of the buffer\n";
    buffer.queueToSstream(outstream);
    cout << outstream.str();
    outstream.str("");

    /* Release semaphores */
    bufferMutex.release(); // up(mutex)
    space.release(); // up(space)



    /* Announce sleep */
    outstream << "Consumer thread with id " << this_thread::get_id()
    << " sleeps for " << queueFront << " seconds\n";
    cout << outstream.str();

    /* "Consume" the item */
    auto sleepTime = chrono::seconds(queueFront);
    this_thread::sleep_for(sleepTime);
}

void producer(CircularQueue& buffer, counting_semaphore<1000>& item, counting_semaphore<1000>& space) {
    /* Announce thread creation */
    stringstream outstream;
    outstream << "Producer thread with id " << this_thread::get_id() << " created\n";
    cout << outstream.str();
    outstream.str("");

    for (int jobsDone = 0; jobsDone < noJobs; jobsDone++) {
        int newNumber = rand() % 10 + 1;
        if (!space.try_acquire_for(10s)) { // down(mutex)
            /* Timeout */
            outstream << "Producer thread with id " << this_thread::get_id()
            << " times out\n";
            cout << outstream.str();
            return;
        }
        bufferMutex.acquire(); // down(mutex)
        buffer.enQueue(newNumber); // Add random int to the front of the queue

	/* Announce successful entry to the buffer */
        outstream << "Producer thread with id " << this_thread::get_id()
        << " adds " << newNumber
        << " to the rear of the buffer\n";
	buffer.queueToSstream(outstream);
        cout << outstream.str();
        outstream.str("");

        /* Release semaphores */
        bufferMutex.release(); // up(mutex)
        item.release(); // up(item)
    }
}

int main(int argc, char* argv[]) {
    /* Test validity of command line args */
    if (argc != 5) { // Includes the executable
        cerr << "Error: submitted " << argc << " arguments: ";
        for (int i = 0; i < argc; i++) cout << argv[i] << " ";
        cerr << "\nMust submit 4 arguments in addition to the executable\n";
        return 1;
    }
    for (int i = 1; i < argc; i++) {
        if (atoi(argv[i]) < 1) {
            cerr << "All command-line args must be at least 1\n";
            return 1;
        }
        if (atoi(argv[i]) >= 1000) {
            cerr << "All command-line args must be below 1000\n";
            return 1;
        }
    }

    /* Initialise variables */
    queueSize = atoi(argv[1]);
    noJobs = atoi(argv[2]);
    noProducers = atoi(argv[3]);
    noConsumers = atoi(argv[4]);

    /* Announce variable initialisation */
    cout << "Queue size: " << queueSize << '\n';
    cout << "Number of jobs: " << noJobs << '\n';
    cout << "Number of producers: " << noProducers << '\n';
    cout << "Number of consumers: " << noConsumers << '\n';
    cout << '\n';

    /* Create circular queue */
    CircularQueue buffer(queueSize);

    /* Create counting semaphores*/
    counting_semaphore<1000> item(0);
    counting_semaphore<1000> space(queueSize);

    /* Producer and consumer vectors */
    vector<thread> all_producers;
    vector<thread> all_consumers;

    /* Populate producer and consumer vectors, and start threads */
    for (int i = 0; i < noProducers; i++) {
      all_producers.push_back(thread(producer, ref(buffer), ref(item), ref(space)));
    }
    for (int i = 0; i < noProducers; i++) {
      all_consumers.push_back(thread(consumer, ref(buffer), ref(item), ref(space)));
    }

    /* Join all threads */
    for (unsigned int i = 0; i < all_producers.size(); i++) {
        all_producers[i].join();
    }
    for (unsigned int i = 0; i < all_producers.size(); i++) {
        all_consumers[i].join();
    }

    buffer.clear();
   
    /* exit */
    return 0;
}
