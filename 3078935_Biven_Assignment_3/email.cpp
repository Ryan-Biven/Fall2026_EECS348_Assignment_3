// EECS 348 Assignment 3
// Description: A C++ program that manages emails.
// Given an input of emails from a text file, along with commands
// NEXT, COUNT AND READ it will print to the console the count of emails
// or read the next email.
// Inputs: None
// Outputs: None
// Colaborators: Ryan Biven
// Sources: Claude and gemini
// Author: Ryan Biven
// Creation Date: 10/1
// Revision Date: 10/1
// Revisions: Tie breaking, using line trimming, email requires a trailing space,
// next on empty has a print, decrement heap size before heapify down,
// pass in file on command line

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>   // IMPROVEMENT: needed so we can open a file given on the command line
#include <stdexcept> // IMPROVEMENT: for throwing/catching bad-date errors

// Class representing an individual Email
class Email {
private:
    std::string sender;
    std::string subject;
    std::string date;     // MM-DD-YYYY
    int senderPriority;   // bigger = more important sender
    int dateSortKey;      // YYYYMMDD so dates compare like normal numbers
    long sequence;        // IMPROVEMENT: arrival order, used to break exact ties

    // turn the category name into a number rank
    int calculateSenderPriority(const std::string& category) const {
        if (category == "Boss") return 5;
        if (category == "Subordinate") return 4;
        if (category == "Peer") return 3;
        if (category == "ImportantPerson") return 2;
        return 1; // OtherPerson (also the fallback for anything unknown)
    }

    // turn MM-DD-YYYY into YYYYMMDD
    int parseDate(const std::string& d) const {
        // IMPROVEMENT: throw on bad dates instead of quietly returning 0,
        // so the caller can skip the bad line
        if (d.length() != 10 || d[2] != '-' || d[5] != '-') {
            throw std::invalid_argument("bad date format");
        }
        int month = std::stoi(d.substr(0, 2));
        int day = std::stoi(d.substr(3, 2));
        int year = std::stoi(d.substr(6, 4));
        // IMPROVEMENT: reject dates that can't exist (like 13-45-2024)
        if (month < 1 || month > 12 || day < 1 || day > 31) {
            throw std::invalid_argument("date out of range");
        }
        return year * 10000 + month * 100 + day;
    }

public:
    // empty email, used as a placeholder
    Email() : senderPriority(0), dateSortKey(0), sequence(0) {}

    // IMPROVEMENT: constructor now also takes the arrival number (seq)
    Email(const std::string& s, const std::string& subj, const std::string& d, long seq)
        : sender(s), subject(subj), date(d), sequence(seq) {
        senderPriority = calculateSenderPriority(s);
        dateSortKey = parseDate(d); // may throw if the date is junk
    }

    // simple getters
    std::string getSender() const { return sender; }
    std::string getSubject() const { return subject; }
    std::string getDate() const { return date; }

    // "this email should be read before other"
    // 1) better sender wins  2) newer date wins  3) earlier arrival wins
    bool operator>(const Email& other) const {
        if (this->senderPriority != other.senderPriority) {
            return this->senderPriority > other.senderPriority;
        }
        if (this->dateSortKey != other.dateSortKey) {
            return this->dateSortKey > other.dateSortKey;
        }
        return this->sequence < other.sequence; // IMPROVEMENT: stable tie-break
    }

    // opposite check, kept in sync with operator> above
    bool operator<(const Email& other) const {
        return other > *this; // IMPROVEMENT: just reuse operator> so they never disagree
    }
};

// Node structure for Doubly Linked List implementation
struct Node {
    Email data;
    Node* prev;
    Node* next;

    Node(const Email& email) : data(email), prev(nullptr), next(nullptr) {}
};

// List-based MaxHeap Implementation
class ListMaxHeap {
private:
    Node* head;    // root of the heap (index 1)
    Node* tail;    // last node in the heap
    int heapSize;

    // walk the list to the node at 1-based index (slow but simple)
    Node* getNodeAt(int index) const {
        if (index < 1 || index > heapSize) return nullptr;

        Node* curr = head;
        for (int i = 1; i < index && curr != nullptr; ++i) {
            curr = curr->next;
        }
        return curr;
    }

    // swap the emails stored in two nodes (nodes stay put)
    void swapNodeValues(Node* a, Node* b) {
        if (a && b) {
            Email temp = a->data;
            a->data = b->data;
            b->data = temp;
        }
    }

    // bubble a new node up until its parent is bigger
    void heapifyUp(int index) {
        while (index > 1) {
            int parentIndex = index / 2;
            Node* currNode = getNodeAt(index);
            Node* parentNode = getNodeAt(parentIndex);

            if (currNode && parentNode && currNode->data > parentNode->data) {
                swapNodeValues(currNode, parentNode);
                index = parentIndex;
            } else {
                break; // parent is bigger, we're done
            }
        }
    }

    // push a node down until both children are smaller
    void heapifyDown(int index) {
        while (2 * index <= heapSize) {
            int leftIndex = 2 * index;
            int rightIndex = 2 * index + 1;
            int largestIndex = index;

            Node* largestNode = getNodeAt(largestIndex);
            Node* leftNode = getNodeAt(leftIndex);
            Node* rightNode = getNodeAt(rightIndex);

            // pick the biggest of node, left child, right child
            if (leftNode && leftNode->data > largestNode->data) {
                largestIndex = leftIndex;
                largestNode = leftNode;
            }

            if (rightNode && rightNode->data > largestNode->data) {
                largestIndex = rightIndex;
                largestNode = rightNode;
            }

            if (largestIndex != index) {
                swapNodeValues(getNodeAt(index), largestNode);
                index = largestIndex; // keep sinking
            } else {
                break; // already in the right spot
            }
        }
    }

public:
    // start with an empty heap
    ListMaxHeap() : head(nullptr), tail(nullptr), heapSize(0) {}

    // free every node when the heap goes away
    ~ListMaxHeap() {
        Node* curr = head;
        while (curr) {
            Node* nextNode = curr->next;
            delete curr;
            curr = nextNode;
        }
    }

    // IMPROVEMENT: block copying so two heaps never delete the same nodes
    ListMaxHeap(const ListMaxHeap&) = delete;
    ListMaxHeap& operator=(const ListMaxHeap&) = delete;

    // add an email: stick it on the end, then bubble it up
    void push(const Email& email) {
        Node* newNode = new Node(email);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        heapSize++;
        heapifyUp(heapSize);
    }

    // look at the top email without removing it
    // IMPROVEMENT: return a const reference so we don't copy the whole Email every time
    const Email& peek() const {
        return head->data; // caller must check isEmpty() first
    }

    // remove the top email
    void pop() {
        if (isEmpty()) return;

        if (heapSize == 1) {
            delete head; // only one node, just free it
            head = tail = nullptr;
            heapSize--;
        } else {
            // move the last email to the root
            swapNodeValues(head, tail);

            // chop off the last node (that's the email we just "read")
            Node* oldTail = tail;
            tail = tail->prev;
            tail->next = nullptr;
            delete oldTail; // memory freed right here

            // IMPROVEMENT: shrink the size BEFORE heapifyDown so it
            // doesn't try to look at a node that's already gone
            heapSize--;

            // fix the heap from the root
            heapifyDown(1);
        }
    }

    bool isEmpty() const {
        return heapSize == 0;
    }

    int size() const {
        return heapSize;
    }
};

// Manager class handling CEO Inbox Queue
class CEOInbox {
private:
    ListMaxHeap priorityQueue;
    long nextSequence; // IMPROVEMENT: counter that stamps each email's arrival order

    // Helper function to trim whitespace
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    CEOInbox() : nextSequence(0) {} // IMPROVEMENT: start the arrival counter at 0

    void processCommand(const std::string& rawLine) {
        // IMPROVEMENT: trim the whole line first so stray spaces or Windows
        // "\r" endings don't break NEXT / READ / COUNT
        std::string line = trim(rawLine);
        if (line.empty()) return;

        // IMPROVEMENT: require "EMAIL " (with the space) so a bare "EMAIL"
        // or something like "EMAILX" doesn't crash or sneak through
        if (line.compare(0, 6, "EMAIL ") == 0) {
            std::string data = line.substr(6); // everything after "EMAIL "
            std::stringstream ss(data);
            std::string sender, subject, date;

            // split on commas into the three fields
            std::getline(ss, sender, ',');
            std::getline(ss, subject, ',');
            std::getline(ss, date, ',');

            // IMPROVEMENT: skip the line if any field is missing
            if (trim(sender).empty() || trim(subject).empty() || trim(date).empty()) {
                return;
            }

            // IMPROVEMENT: catch bad dates so one junk line can't crash the program
            try {
                Email newEmail(trim(sender), trim(subject), trim(date), nextSequence);
                priorityQueue.push(newEmail);
                nextSequence++; // only count it if it actually got added
            } catch (const std::exception&) {
                // bad date, just ignore this email
            }
        }
        else if (line == "NEXT") {
            if (!priorityQueue.isEmpty()) {
                // show the top email without removing it
                const Email& top = priorityQueue.peek();
                std::cout << "Next email:\n";
                std::cout << "Sender: " << top.getSender() << "\n";
                std::cout << "Subject: " << top.getSubject() << "\n";
                std::cout << "Date: " << top.getDate() << "\n";
            } else {
                // IMPROVEMENT: tell the user the inbox is empty instead of printing nothing
                std::cout << "No emails to read.\n";
            }
        }
        else if (line == "READ") {
            // remove the top email (does nothing if empty)
            if (!priorityQueue.isEmpty()) {
                priorityQueue.pop();
            }
        }
        else if (line == "COUNT") {
            std::cout << "There are " << priorityQueue.size() << " emails to read.\n";
        }
    }
};

// IMPROVEMENT: main now accepts a file name, but still falls back to stdin
int main(int argc, char* argv[]) {
    CEOInbox inbox;
    std::string line;

    if (argc > 1) {
        // file name given: open it and read line by line
        std::ifstream in(argv[1]);
        if (!in) {
            std::cerr << "Could not open file: " << argv[1] << "\n";
            return 1;
        }
        while (std::getline(in, line)) {
            inbox.processCommand(line);
        }
    } else {
        // no file name: read from the keyboard / redirected input like before
        while (std::getline(std::cin, line)) {
            inbox.processCommand(line);
        }
    }

    return 0;
}