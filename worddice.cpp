/*
    Project 5 - worddice.cpp
    Jordan Huff and Austin Tran
    4/21/23
    COSC 302

    This program generates a graph of nodes and edges based off of input of words and dice containing letters that are used to spell the word.
    In order to assign the dice to the letters, the Edmonds-Karp algorithm was implemented to find the optimal paths between them.
    The output is the result of Edmonds-Karp: whether or not the word can be spelled and if it can, the order of dice for spelling.
*/

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <fstream>
#include <bits/stdc++.h>
#include <queue>
#include <algorithm>

using namespace std;

// edges in between nodes - taken from Rob's slides with minor edits
class Edge {
    public:
    
    //from -> to
    class Node *to; //node edge is pointing to
    class Node *from; //node edge is pointing from
    Edge(Node *to_, Node *from_, bool reverse_edge = false); // constructor for edges
    ~Edge(){}; //default destructor
    
    Edge *reverse; //edge going the other way
    int original; //weight per edge

};

// types of nodes - taken from Rob's slides with minor edits
typedef enum{SOURCE, SINK, WORD, DICE} Node_Type;

// nodes within graph - taken from Rob's slides with minor edits
class Node {
    public:

    Node(int id, Node_Type type, string word = ""); //constructor for nodes
    ~Node(); //destructor

    int id; //node id
    Node_Type type; //type of node it is (source, sink, word or dice)
    vector<bool> letters; //length 26 with letters contained in word set to 1
    int visited; //for BFS
    vector<Edge*> adj; //adjacency list
    Edge *backedge; //previous edge for Edmonds-Karp
};

// graph of nodes and edges - taken from Rob's slides with minor edits
class Graph {
    public:

    Graph(); //constructor initializes graph with source node
    ~Graph(); //destructor to deallocate memory of graph

    Node *source; //not necessary but makes code more readable
    Node *sink;    
    vector<Node *> nodes; //holds the nodes    

    vector<int> spellingIds; //order of flow to spell word
    int letter_num = 0; //min number of dice nodes    
    string word;

    void add_dice_to_graph(string die, int id); //add dice nodes to graph    
    void add_word_to_graph(string word, int& id); //add word (letter) nodes to graph    
    bool BFS(); //breadth first search for Edmonds-Karp    
    bool spell_word(); //runs Edmonds-Karp to see if we can spell the word
};

// Edge constructor
Edge::Edge(Node *to_, Node *from_, bool reverse_edge) {
    // sets adjacent nodes
    this->to = to_;
    this->from = from_;

    if (reverse_edge) {
        original = 0;
    }
    else {
        original = 1;
    }
}

// Node constructor: sets initial information
Node::Node(int id, Node_Type type, string word) {
    this->id = id;
    this->type = type;

    letters = vector<bool>(26,0);
    
    for (size_t i = 0; i < word.size(); i++) {
        this->letters[word[i] - 'A'] = true;
    }
    
    visited = 0;
}

// Node deconstructor: deletes all edges in node adjacency list
Node::~Node() {
    for (size_t i = 0; i < adj.size(); i++) {
        delete adj[i];
    }
}

// Graph constructor: sets source
Graph::Graph() {
    source = new Node(0, SOURCE);

    nodes.push_back(source);
}

// Graph deconstructor: deletes all nodes
Graph::~Graph() {
    for (size_t i = 0; i < nodes.size(); i++) {
        delete nodes[i];
    }
}

// adds dice nodes to graph 
void Graph::add_dice_to_graph(string die, int id){
    Node* new_dice = new Node(id, DICE, die);

    // edge moving from source to new_dice
    Edge* new_edge1 = new Edge(new_dice, source, false);
    
    // edge moving from new_dice to source
    Edge* new_edge2 = new Edge(source, new_dice, true);

    // edges set to be the reverse of each other
    new_edge1->reverse = new_edge2;
    new_edge2->reverse = new_edge1;

    // edge moving away from new_dice added to its adjacency list
    new_dice->adj.push_back(new_edge2);

    // push back reverse onto sources
    source->adj.push_back(new_edge1);

    nodes.push_back(new_dice);
}

// adds word (letter) nodes to graph
void Graph::add_word_to_graph(string word, int& id){

    Edge *new_edge1, *new_edge2;

    // loops through letters in word, adding nodes
    for (size_t i = 0; i < word.size(); i++) {

        // sets number of letters in current word
        letter_num++;

        Node* new_letter = new Node(id + i, WORD, word);

        // set edges for every die
        for (size_t j = 0; j < nodes.size(); j++) {
            
            // sets edges if current node is a DICE node and if the letters match with the current letter node
            if ((nodes[j]->type == DICE) && (nodes[j]->letters[word[i] - 'A'] == 1)) {
                new_edge1 = new Edge(new_letter, nodes[j], false);
                new_edge2 = new Edge(nodes[j], new_letter, true);

                new_edge1->reverse = new_edge2;
                new_edge2->reverse = new_edge1;

                new_letter->adj.push_back(new_edge2);

                nodes[j]->adj.push_back(new_edge1);

            }
        }

        nodes.push_back(new_letter);
    }

    // initializes sink for every letter added
    sink = new Node(id + word.size(), SINK);

    // sets edges for sink and adjacent WORD nodes
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->type == WORD) {
            new_edge1 = new Edge(nodes[i], sink, true);
            new_edge2 = new Edge(sink, nodes[i], false);

            new_edge1->reverse = new_edge2;
            new_edge2->reverse = new_edge1;

            nodes[i]->adj.push_back(new_edge2);
            sink->adj.push_back(new_edge1);
        }
    }

    nodes.push_back(sink);
}

// runs an iteration of BFS to set weights and backedges
bool Graph::BFS(){

    Node *current = source;         // iterative node
    queue<Node*> frontier;          // queue of nodes, FIFO
    list<Node*> visited;            // all nodes visited
    list<Node*>::iterator lit;      // visited iterator

    // initializes frontier with source node
    frontier.push(current);

    // loops until the frontier is empty
    while (frontier.size() != 0){
        
        // FIFO implementation of frontier
        current = frontier.front();
        frontier.pop();

        // loops through adjacency list of current node, setting backedges if it is a valid path
        for (size_t i = 0; i < current->adj.size(); i++) {
            lit = find(visited.begin(), visited.end(), current->adj[i]->to);
            
            // makes sure current adjacency edge isn't untraversable or already visisted
            if (current->adj[i]->original == 0 || lit != visited.end()) continue;

            // sets backedges
            current->adj[i]->to->backedge = current->adj[i];

            // adds valid nodes to visited and frontier
            visited.insert(lit, current->adj[i]->to);
            frontier.push(current->adj[i]->to);
        }

        // if the sink is reached, BFS is successful
        if (current->type == SINK) return true;
    }
    
    // if loop ends without reaching sink, BFS is unsuccessful
    return false;
}

// runs Edmonds-Karp to see if we can spell the word
bool Graph::spell_word(){
    int success = 0;    // number of successful BFS runs
    Node *current;      // iterative node


    // runs BFS for as many letters as exist to try to find a match for every letter
    for (int i = 0; i < letter_num; i++) {

        // if BFS is successful, the backedges are looped through and the weights of the edges are set
        if (BFS() == true) {
            current = sink;
            while(current->type != SOURCE) {

                current->backedge->original = 0;
                current->backedge->reverse->original = 1;

                current = current->backedge->from;
            }

            success++;
        }
    }

    // if there are less matches than letters, the word can't be spelled
    if (success < letter_num) return false;
    
    // otherwise, it can be
    else return true;

}


int main(int argc, char *argv[]) { 
    string temp;                // temporary string
    vector<string> dice_vec;    // vector of dice strings
    vector<string> word_vec;    // vector of word strings
    
    // opens dice file and grabs every dice string
    ifstream dice_file(argv[1]);
    if (dice_file.is_open()) {
        while(getline(dice_file, temp)) {
            dice_vec.push_back(temp);
        }
    }
    dice_file.close();
    
    // opens word file and grabs every word
    ifstream word_file (argv[2]);
    if (word_file.is_open()) {
        int it = 0;
        while(getline(word_file, temp)) {
            word_vec.push_back(temp);
            it++;
        }
    }
    word_file.close();
    
    // loops through all words, creating a graph with it and all the dice and running edmond's karp on it
    for (size_t i = 0; i < word_vec.size(); i++) {
        
        int id = 1;         // the id of nodes, which is incremented every time a node is inserted
        Graph wordGraph;    // the graph of nodes and edges
        size_t word_it = 0; // iterator for the current letter being printed

        // adds dice and letters to the graph
        for(size_t i = 0; i < dice_vec.size(); i++){
            wordGraph.add_dice_to_graph(dice_vec[i], id);
            id++;
        }
        wordGraph.add_word_to_graph(word_vec[i], id);

        // output if edmond's karp results in a failure
        if (wordGraph.spell_word() == false) {
            cout << "Cannot spell " << word_vec[i] << '\n';
        }

        // output if edmond's karp results in a success
        else {

            // loops through all nodes, printing out the id of each DICE node connected to every WORD node
            for (size_t j = 0; j < wordGraph.nodes.size(); j++) {
                
                // checks if current node is a WORD node
                if (wordGraph.nodes[j]->type == WORD) {
                    word_it++;

                    // loops through adjacency list of WORD node
                    for (size_t k = 0; k < wordGraph.nodes[j]->adj.size(); k++) {
                        
                        // prints corresponding DICE node ids in order
                        if (wordGraph.nodes[j]->adj[k]->to->type == DICE && wordGraph.nodes[j]->adj[k]->original == 1) {
                            cout << wordGraph.nodes[j]->adj[k]->to->id - 1;
                            if (word_it < word_vec[i].size()) {
                                cout << ',';
                            }
                        }
                    }
                }
            }

            cout << ": " << word_vec[i] << '\n';
        }
    }

    return 0;
}