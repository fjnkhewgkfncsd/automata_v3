#include <iostream>
#include <set>
#include <map>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <cstdio>  
#include <iomanip>
#include <vector>
#include <chrono>
#include <thread>
#include <windows.h>
#include<unistd.h>

using namespace std;

void clearScreen() {
    #ifdef _WIN32
        system("cls");     // Windows
    #else
        system("clear");   // Unix/Linux/macOS
    #endif
}

void pauseAndClear() {
    cout << "\nPress Enter to continue...";
    cin.ignore();
    cin.get();
    clearScreen();
}

void sleepFor(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);  // Windows - milliseconds
#else
    usleep(milliseconds * 1000);  // Unix - microseconds
#endif
}

class FiniteAutoMaton {
    protected:
        set<string> states;
        set<char> alphabets;
        string startState;
        set<string> acceptingStates;
        string name;
        int numOfStates;
        int numOfAlphabet;
        int numOfAcceptingStates;
        int id;
    public:
        virtual void loadFromDatabase(int id) = 0;
        virtual bool simulate(const string& input) = 0;
        virtual void saveToDatabase() = 0;
        virtual ~FiniteAutoMaton() = default;
        virtual void displayTransitions() = 0;  
        virtual void addSymbol(char symbol){
            alphabets.insert(symbol);
        }
        virtual void addAcceptingStates(const string& state){
            acceptingStates.insert(state);
        }
        virtual void displayState() const {
            for(const auto& state : states){
                if(state == "nt"){
                    continue;
                }
                if(state ==startState){
                    cout << state +"* ";
                }else{
                    cout << state << " ";
                }
            }
        }
        virtual void displaySymbol(){
            for(const char symbol : alphabets){
                cout << symbol << " ";
            }
        }
        virtual void addStates(string& state){
            states.insert(state);
        }
        virtual void setNumOfState(int num){
            numOfStates = num;
        }
        virtual void setNumOfAlphabet(int num){
            numOfAlphabet = num;
        }
        virtual int getNumOfState(){
            return numOfStates;
        }
        virtual int getNumOfAlphabet(){
            return numOfAlphabet;
        }
        virtual set<string>& getStates(){
            return states;
        }
        virtual set<char> getAlphabet(){
            return alphabets;
        } 
        virtual void setStartState(const string& state){
            startState = state ;
        }
        virtual void setAcceptingStates(const string& state){
            acceptingStates.insert(state);
        }
        virtual void setNumOfAcceptingState(int num){
            numOfAcceptingStates = num;
        }
        virtual int getNumOfAcceptingState(){
            return numOfAcceptingStates;
        }
        map<int,string> listAvailableFA(){
            string command = "python db_operation.py list";
            cout << " Loading available finite automata..." << endl;
            sleepFor(2000);
            clearScreen();
            map<int,string> faTypes;
            FILE* pipe = popen(command.c_str(), "r");
            if (!pipe) {
                cout << " Failed to execute Python script!" << endl;
                return faTypes;
            }
            
            char buffer[1024];
            string result;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            pclose(pipe);

            if (result.find("EMPTY") != string::npos) {
                cout << "No finite automata found in database." << endl;
                return faTypes;
            }

            try {
                size_t automataStart = result.find("\"automata\":");
                if (automataStart == string::npos) {
                    cout << " Error: Could not find automata array in response" << endl;
                    return faTypes;
                }

                // Print clean header
                cout << "\n Available Finite Automata:" << endl;
                cout << string(50, '=') << endl;
                cout << left << setw(6) << "ID" << "| " 
                    << setw(8) << "Type" << "| "
                    << "Name" << endl;
                cout << string(50, '=') << endl;

                // Find array bounds
                size_t arrayStart = result.find('[', automataStart);
                size_t arrayEnd = result.find(']', arrayStart);
                
                if (arrayStart == string::npos || arrayEnd == string::npos) {
                    cout << " Error: Invalid JSON format" << endl;
                    return faTypes;
                }
        string jsonArray = result.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
        size_t pos = 0;
        
        while ((pos = jsonArray.find('{', pos)) != string::npos) {
            size_t endPos = jsonArray.find('}', pos);
            if (endPos == string::npos) break;

            string item = jsonArray.substr(pos, endPos - pos + 1);
            
            string idStr = extractField(item, "id");
            string type = extractField(item, "type");
            string name = extractField(item, "name");

            if (!idStr.empty()){
                int id = stoi(idStr);
                faTypes[id] = type.empty() ? "DFA" : type; // Default to DFA if empty
            }
            // Print clean formatted row
            cout << left << setw(6) << idStr << "| "
                 << setw(8) << type << "| "
                 << name << endl;

            pos = endPos + 1;
        }
        cout << string(50, '=') << endl;
        
    } catch (const exception& e) {
        cout << "❌ Error parsing response: " << e.what() << endl;
    } catch (...) {
        cout << "❌ Unknown error parsing response" << endl;
    }
    return faTypes;
}


// Clean extractField function without debug
string extractField(const string& json, const string& field) {
    string pattern = "\"" + field + "\": \"";
    size_t start = json.find(pattern);
    
    if (start == string::npos) {
        return "";
    }
    
    start += pattern.length();
    size_t end = json.find("\"", start);
    
    if (end == string::npos) {
        return "";
    }
    
    return json.substr(start, end - start);
}
};

class DFA : public FiniteAutoMaton {
    private: 
        map<pair<string,char>, string> transitions;
        
    public:
        string toJSON(const string& name) const {
            stringstream json;
            json << "{\n";
            json << "  \"type\": \"DFA\",\n";
            json << "  \"name\": \"" << name << "\",\n";
            json << "  \"numOfStates\": " << numOfStates << ",\n";
            json << "  \"numOfAlphabet\": " << numOfAlphabet << ",\n";
            json << "  \"numOfAcceptingStates\": " << numOfAcceptingStates << ",\n";
            
            // States array
            json << "  \"states\": [";
            bool first_state = true;
            for (const auto& state : states) {
                if (!first_state) json << ", ";
                json << "\"" << state << "\"";
                first_state = false;
            }
            json << "],\n";
            
            // Start state
            json << "  \"startState\": \"" << startState << "\",\n";
            
            // Alphabet array
            json << "  \"alphabet\": [";
            bool first_symbol = true;
            for (const auto& symbol : alphabets) {
                if (!first_symbol) json << ", ";
                json << "\"" << string(1, symbol) << "\"";
                first_symbol = false;
            }
            json << "],\n";
            json << "  \"acceptingStates\": [";
            bool first_acceptingState = true;
            for (const auto& state : acceptingStates) {
                if (!first_acceptingState) json << ", ";
                json << "\"" << state << "\"";
                first_acceptingState = false;
            }
            json << "],\n";
            json << "  \"transitions\": [";
            bool first_transition = true;
            for (const auto& transition : transitions) {
                if (!first_transition) json << ", ";
                // transition.first is pair<string,char>, transition.second is string
                json << "[\"" << transition.first.first << "\", \"" 
                        << string(1, transition.first.second) << "\", \"" 
                        << transition.second << "\"]";
                first_transition = false;
            }
            json << "]\n";
            json << "}";
            return json.str();
        }
        // Parse JSON and populate DFA
        bool fromJSON(const string& jsonFile) {
            ifstream file(jsonFile);
            if (!file.is_open()) {
                cout << "❌ Failed to open JSON file: " << jsonFile << endl;
                return false;
            }
            string line, content;
            while (getline(file, line)) {
                content += line;
            }
            file.close();
            try {
                // Clear existing data
                states.clear();
                alphabets.clear();
                acceptingStates.clear();
                transitions.clear();
                // Extract basic info
                name = extractField(content, "name");
                startState = extractField(content, "startState");
                
                // Extract and parse states array
                size_t statesStart = content.find("\"states\": [");
                if (statesStart != string::npos) {
                    size_t arrayStart = content.find('[', statesStart);
                    size_t arrayEnd = content.find(']', arrayStart);
                    string statesArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    
                    
                    size_t pos = 0;
                    while ((pos = statesArray.find('"', pos)) != string::npos) {
                        pos++;
                        size_t endPos = statesArray.find('"', pos);
                        if (endPos != string::npos) {
                            string state = statesArray.substr(pos, endPos - pos);
                            states.insert(state);
                            pos = endPos + 1;
                        } else {
                            break; // Prevent infinite loop
                        }
                    }
                }
                
                // Extract and parse alphabet array
                size_t alphabetStart = content.find("\"alphabet\": [");
                if (alphabetStart != string::npos) {
                    size_t arrayStart = content.find('[', alphabetStart);
                    size_t arrayEnd = content.find(']', arrayStart);
                    string alphabetArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    
                    
                    size_t pos = 0;
                    while ((pos = alphabetArray.find('"', pos)) != string::npos) {
                        pos++;
                        size_t endPos = alphabetArray.find('"', pos);
                        if (endPos != string::npos) {
                            string symbol = alphabetArray.substr(pos, endPos - pos);
                            if (!symbol.empty()) {
                                alphabets.insert(symbol[0]);
                            }
                            pos = endPos + 1;
                        } else {
                            break; // Prevent infinite loop
                        }
                    }
                }
                
                // Extract and parse accepting states
                size_t acceptingStart = content.find("\"acceptingStates\": [");
                if (acceptingStart != string::npos) {
                    size_t arrayStart = content.find('[', acceptingStart);
                    size_t arrayEnd = content.find(']', arrayStart);
                    string acceptingArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    
                    
                    size_t pos = 0;
                    while ((pos = acceptingArray.find('"', pos)) != string::npos) {
                        pos++;
                        size_t endPos = acceptingArray.find('"', pos);
                        if (endPos != string::npos) {
                            string state = acceptingArray.substr(pos, endPos - pos);
                            acceptingStates.insert(state);
                            pos = endPos + 1;
                        } else {
                            break; // Prevent infinite loop
                        }
                    }
                }
                
                // Extract and parse transitions - FIXED VERSION
                size_t transitionsStart = content.find("\"transitions\": [");
                if (transitionsStart != string::npos) {
                    size_t arrayStart = content.find('[', transitionsStart);
                    size_t arrayEnd = content.find(']', arrayStart);
                    
                    // Find the LAST ']' to get the complete transitions array
                    size_t lastBracket = arrayStart;
                    int bracketCount = 1;
                    for (size_t i = arrayStart + 1; i < content.length() && bracketCount > 0; i++) {
                        if (content[i] == '[') bracketCount++;
                        else if (content[i] == ']') {
                            bracketCount--;
                            if (bracketCount == 0) {
                                arrayEnd = i;
                                break;
                            }
                        }
                    }
                    
                    string transitionsArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    
                    
                    // Parse each transition array [from, symbol, to]
                    size_t pos = 0;
                    int transitionCount = 0;
                    
                    while ((pos = transitionsArray.find('[', pos)) != string::npos) {
                        size_t endPos = transitionsArray.find(']', pos);
                        if (endPos == string::npos) break;
                        
                        string transition = transitionsArray.substr(pos + 1, endPos - pos - 1);
                        
                        // Extract the three parts: from, symbol, to
                        vector<string> parts;
                        size_t quotePos = 0;
                        
                        // Find all quoted strings in this transition
                        while ((quotePos = transition.find('"', quotePos)) != string::npos) {
                            quotePos++; // Skip opening quote
                            size_t endQuote = transition.find('"', quotePos);
                            if (endQuote != string::npos) {
                                string part = transition.substr(quotePos, endQuote - quotePos);
                                parts.push_back(part);
                                quotePos = endQuote + 1;
                            } else {
                                break;
                            }
                        }
                        
                        // Should have exactly 3 parts: [from, symbol, to]
                        if (parts.size() == 3) {
                            string from = parts[0];
                            char symbol = parts[1][0]; // Take first character
                            string to = parts[2];
                            
                            transitions[{from, symbol}] = to;
                            transitionCount++;
                        } else {
                            for (size_t i = 0; i < parts.size(); i++) {
                                cout << "DEBUG: Part " << i << ": '" << parts[i] << "'" << endl;
                            }
                        }
                        
                        pos = endPos + 1;
                    }
                    
                }
                
                
                // Update counts
                numOfStates = states.size();
                numOfAlphabet = alphabets.size();
                numOfAcceptingStates = acceptingStates.size();
                
                return true;
            } catch (...) {
                cout << " Error parsing JSON file" << endl;
                return false;
            }
        }
        
        void saveToDatabase() override {
            string dfaName;
            cout << "Enter a name for this DFA: ";
            cin >> dfaName;
            
            // Create JSON file
            string jsonData = toJSON(dfaName);
            string tempFile = "temp_dfa_save.json";
            
            ofstream jsonFile(tempFile);
            if (!jsonFile.is_open()) {
                cout << " Failed to create temporary JSON file!" << endl;
                return;
            }
            jsonFile << jsonData;
            jsonFile.close();
            
            // Execute Python script
            string command = "python db_operation.py insert " + tempFile;
            cout << " Saving DFA to database..." << endl;
            
            // Use simple system() call instead of popen
            int result = system(command.c_str());
            
            // Cleanup temporary file
            remove(tempFile.c_str());
            
            // Just check the result
            if (result == 0) {
                cout << " DFA '" << dfaName << "' saved to database successfully!" << endl;
            } else {
                cout << " Failed to save DFA to database!" << endl;
            }
        }
        
        void loadFromDatabase(int id) override {
            string tempFile = "temp_dfa_load.json";
            string command = "python db_operation.py load " + to_string(id) + " " + tempFile;
            
            cout << "Loading DFA from database (ID: " << id << ")..." << endl;
            
            int result = system(command.c_str());
            
            if (result == 0) {
                // Check if file was created successfully
                ifstream checkFile(tempFile);
                if (checkFile.good()) {
                    checkFile.close();
                    if (fromJSON(tempFile)) {
                        cout << " DFA loaded successfully from database!" << endl;
                        displayTransitions();
                    } else {
                        cout << " Failed to parse loaded DFA data!" << endl;
                    }
                    // Cleanup
                    remove(tempFile.c_str());
                } else {
                    cout << " DFA with ID " << id << " not found in database!" << endl;
                }
            } else {
                cout << " Error loading DFA from database!" << endl;
            }
        }
        
        bool simulate(const string& input) override {
            string currentState = startState;
            cout << " Simulating input: '" << input << "'" << endl;
            cout << "  Start state: " << currentState << endl;
            
            for (char symbol : input) {
                if (alphabets.find(symbol) == alphabets.end()) {
                    cout << " Symbol '" << symbol << "' not in alphabet!" << endl;
                    return false;
                }
                
                auto transition = transitions.find({currentState, symbol});
                if (transition == transitions.end()) {
                    cout << " No transition from " << currentState << " with symbol " << symbol << endl;
                    return false;
                }
                
                cout << "   " << currentState << " --" << symbol << "--> " << transition->second << endl;
                currentState = transition->second;
            }
            
            bool accepted = acceptingStates.find(currentState) != acceptingStates.end();
            cout << " Final state: " << currentState;
            cout << " (" << (accepted ? " ACCEPTED" : " REJECTED") << ")" << endl;
            return accepted;
        }
        
        void addTransition(const string& from, char symbol, const string& to) {
            transitions[{from, symbol}] = to;
        }
        void displayTransitions() override {
            cout << "\n Transition Table:" << endl;
            cout << "-------------------" << endl;
            for (const auto& transition : transitions) {
                if(transition.first.first ==startState ){
                    cout << transition.first.first + "*" << " --" << transition.first.second 
                        << "--> " << transition.second << endl;
                }else{
                    cout << transition.first.first << " --" << transition.first.second 
                    << "--> " << transition.second << endl;
                }
            }
            cout << endl;
        }
        // Getters for database operations
        map<pair<string,char>, string>& getTransitions(){ return transitions; }
        const string& getStartState() const { return startState; }
        const set<string>& getAcceptingStates() const { return acceptingStates; }
        void handleInputForDFA(){
            cout << "======== Designing DFA =========" << endl;
            cout << "Enter number of states : ";
            int numState;
            cin >> numState;
            numOfStates = numState;
            for(int i = 0 ; i < numOfStates ; i++){
                string state = "q"+ to_string(i);
                addStates(state);
            }
            int numAlphabet;
            cout << "Enter number of symbols in alphabet : ";
            cin >> numAlphabet;
            numOfAlphabet = numAlphabet;
            for(int i = 0 ; i < numOfAlphabet ; i++){
                char symbol;
                cout << "Enter symbol "<< i+1 << ": ";
                cin >> symbol;
                addSymbol(symbol);
            } 
            cout << "You have " <<numOfStates << " states there're ";
            displayState(); 
            cout <<" and "<< numOfAlphabet << " symbols there're ";
            displaySymbol();
            cout <<"in your DFA."<<endl;
            string startstate;
            do{
                cout << "Enter start state : ";
                cin >> startstate;
                if(states.find(startstate) == states.end()){
                    cout << "Error : Start state must be one of the defined states."<<endl;
                }
            }while(states.find(startstate) == states.end());
            startState = startstate;
            int acceptingState;
            cout << "Enter number of accepting states : ";
            cin >> acceptingState;
            numOfAcceptingStates = acceptingState ;
            for(int i =0 ; i < numOfAcceptingStates ; i++){
                string acceptingState;
                do{
                    cout << "Enter accepting states " << i+1 << " : ";
                    cin >> acceptingState;
                    if(states.find(acceptingState) == states.end()){
                        cout << "Error: Accepting state must be one of the defined states." << endl;
                    } 
                }while(states.find(acceptingState) == states.end());  
                addAcceptingStates(acceptingState);
            }
            for(const auto& state : states){
                for(const auto& alphabet : alphabets){
                    string toState;
                    do{
                        cout << "Enter transition from state "<<state << " with symbol "<<alphabet << " to state : ";
                        cin >> toState;
                        if(states.find(toState)==states.end()){
                            cout << "Error: Transition state must be one of the defined states."<<endl;
                        }
                    }while(states.find(toState) == states.end());
                    addTransition(state, alphabet, toState);
                }
            }
            
            // ✅ ADDED: Test the DFA and offer to save to database
            cout << "\n DFA created successfully!" << endl;
            displayTransitions();
            pauseAndClear();
            // Test the DFA
            char testChoice;
            cout << "\n🧪 Do you want to test the DFA? (y/n): ";
            cin >> testChoice;
            if(testChoice == 'y' || testChoice == 'Y') {
                string testInput;
                do {
                    cout << "Enter a string to test (or 'quit' to stop): ";
                    cin >> testInput;
                    if(testInput != "quit") {
                        cout << "\n" << string(30, '-') << endl;
                        if(simulate(testInput)) {
                            cout << "🎉 ACCEPTED!" << endl;
                        } else {
                            cout << "💥 REJECTED!" << endl;
                        }
                        cout << string(30, '-') << endl;
                    }
                } while(testInput != "quit");
            }
                sleepFor(500);
                pauseAndClear();
            // Save to database
            char saveChoice;
            cout << "\n💾 Do you want to save this DFA to database? (y/n): ";
            cin >> saveChoice;
            if(saveChoice == 'y' || saveChoice == 'Y') {
                saveToDatabase();
            }
}
};


class NFA : public FiniteAutoMaton {
    private:
        map<pair<string,char>, set<string>> transitions;
        bool isAllowEpsilonTransitions = false; 
    public:
        map<pair<string,char>, set<string>>& getNFATransitions() { 
            return transitions; 
        }
    
        // ✅ ADD: Convert NFA to JSON (similar to DFA but handles multiple transitions)
        string toJSON(const string& name) const {
            stringstream json;
            json << "{\n";
            json << "  \"type\": \"NFA\",\n";
            json << "  \"name\": \"" << name << "\",\n";
            json << "  \"numOfStates\": " << numOfStates << ",\n";
            json << "  \"numOfAlphabet\": " << numOfAlphabet << ",\n";
            json << "  \"numOfAcceptingStates\": " << numOfAcceptingStates << ",\n";
            
            // States array
            json << "  \"states\": [";
            bool first_state = true;
            for (const auto& state : states) {
                if (!first_state) json << ", ";
                json << "\"" << state << "\"";
                first_state = false;
            }
            json << "],\n";
            
            // Start state
            json << "  \"startState\": \"" << startState << "\",\n";
            
            // Alphabet array
            json << "  \"alphabet\": [";
            bool first_symbol = true;
            for (const auto& symbol : alphabets) {
                if (!first_symbol) json << ", ";
                json << "\"" << string(1, symbol) << "\"";
                first_symbol = false;
            }
            json << "],\n";
            
            // Accepting states array
            json << "  \"acceptingStates\": [";
            bool first_accepting = true;
            for (const auto& state : acceptingStates) {
                if (!first_accepting) json << ", ";
                json << "\"" << state << "\"";
                first_accepting = false;
            }
            json << "],\n";
            
            // ✅ DIFFERENT: NFA transitions (multiple destinations per state-symbol)
            json << "  \"transitions\": [";
            bool first_transition = true;
            for (const auto& transition : transitions) {
                const string& fromState = transition.first.first;
                const char& symbol = transition.first.second;
                const set<string>& toStates = transition.second;
                
                // Create separate transition entry for each destination
                for (const string& toState : toStates) {
                    if (!first_transition) json << ", ";
                    json << "[\"" << fromState << "\", \"" 
                        << string(1, symbol) << "\", \"" 
                        << toState << "\"]";
                    first_transition = false;
                }
            }
            json << "]\n";
            json << "}";
            
            return json.str();
        }
        
        // Parse JSON and populate NFA
        bool fromJSON(const string& jsonFile) {
            
            ifstream file(jsonFile);
            if (!file.is_open()) {
                cout << " Failed to open JSON file: " << jsonFile << endl;
                return false;
            }
            string line, content;
            while (getline(file, line)) {
                content += line;
            }
            file.close();
            
            try {
                // Clear existing data
                states.clear();
                alphabets.clear();
                acceptingStates.clear();
                transitions.clear();
                
                // Extract basic info
                name = extractField(content, "name");
                startState = extractField(content, "startState");
                
                // ✅ FIXED: Extract and parse states array
                size_t statesStart = content.find("\"states\": [");
                if (statesStart != string::npos) {
                    size_t arrayStart = content.find('[', statesStart);
                    size_t arrayEnd = content.find(']', arrayStart);
                    string statesArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    
                    size_t pos = 0;
                    while ((pos = statesArray.find('"', pos)) != string::npos) {
                        pos++;
                        size_t endPos = statesArray.find('"', pos);
                        if (endPos != string::npos) {
                            string state = statesArray.substr(pos, endPos - pos);
                            // ✅ FIXED: Skip "nt" (no transition) states
                            if (state != "nt") {
                                states.insert(state);
                            }
                            pos = endPos + 1;
                        } else {
                            break;
                        }
                    }
                }
                
                // ✅ FIXED: Extract and parse alphabet array
                size_t alphabetStart = content.find("\"alphabet\": [");
                if (alphabetStart != string::npos) {
                    size_t arrayStart = content.find('[', alphabetStart);
                    size_t arrayEnd = content.find(']', arrayStart);
                    string alphabetArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    
                    size_t pos = 0;
                    while ((pos = alphabetArray.find('"', pos)) != string::npos) {
                        pos++;
                        size_t endPos = alphabetArray.find('"', pos);
                        if (endPos != string::npos) {
                            string symbol = alphabetArray.substr(pos, endPos - pos);
                            if (!symbol.empty()) {
                                alphabets.insert(symbol[0]);
                            }
                            pos = endPos + 1;
                        } else {
                            break;
                        }
                    }
                }
                
                // ✅ FIXED: Extract and parse accepting states
                size_t acceptingStart = content.find("\"acceptingStates\": [");
                if (acceptingStart != string::npos) {
                    size_t arrayStart = content.find('[', acceptingStart);
                    size_t arrayEnd = content.find(']', arrayStart);
                    string acceptingArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    
                    size_t pos = 0;
                    while ((pos = acceptingArray.find('"', pos)) != string::npos) {
                        pos++;
                        size_t endPos = acceptingArray.find('"', pos);
                        if (endPos != string::npos) {
                            string state = acceptingArray.substr(pos, endPos - pos);
                            acceptingStates.insert(state);
                            pos = endPos + 1;
                        } else {
                            break;
                        }
                    }
                }
                
                // ✅ FIXED: Extract and parse transitions (INCLUDING EPSILON)
                size_t transitionsStart = content.find("\"transitions\": [");
                if (transitionsStart != string::npos) {
                    size_t arrayStart = content.find('[', transitionsStart);
                    size_t arrayEnd = content.find(']', arrayStart);
                    
                    // Find the complete transitions array
                    int bracketCount = 1;
                    for (size_t i = arrayStart + 1; i < content.length() && bracketCount > 0; i++) {
                        if (content[i] == '[') bracketCount++;
                        else if (content[i] == ']') {
                            bracketCount--;
                            if (bracketCount == 0) {
                                arrayEnd = i;
                                break;
                            }
                        }
                    }
                    
                    string transitionsArray = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    
                    // Parse each transition [from, symbol, to]
                    size_t pos = 0;
                    int transitionCount = 0;
                    
                    while ((pos = transitionsArray.find('[', pos)) != string::npos) {
                        size_t endPos = transitionsArray.find(']', pos);
                        if (endPos == string::npos) break;
                        
                        string transition = transitionsArray.substr(pos + 1, endPos - pos - 1);
                        
                        // Extract the three parts: from, symbol, to
                        vector<string> parts;
                        size_t quotePos = 0;
                        
                        while ((quotePos = transition.find('"', quotePos)) != string::npos) {
                            quotePos++; // Skip opening quote
                            size_t endQuote = transition.find('"', quotePos);
                            if (endQuote != string::npos) {
                                string part = transition.substr(quotePos, endQuote - quotePos);
                                parts.push_back(part);
                                quotePos = endQuote + 1;
                            } else {
                                break;
                            }
                        }
                        
                        // Should have exactly 3 parts: [from, symbol, to]
                        if (parts.size() == 3) {
                            string from = parts[0];
                            string symbolStr = parts[1];
                            string to = parts[2];
                            
                            // ✅ FIXED: Skip "nt" (no transition) entries
                            if (from == "nt" || to == "nt") {
                                pos = endPos + 1;
                                continue;
                            }
                            
                            // ✅ FIXED: Handle epsilon transitions
                            if (symbolStr == "e" || symbolStr == "e" || symbolStr == "epsilon") {
                                addTransition(from, 'e', to);  // Use 'e' for epsilon
                                isAllowEpsilonTransitions = true;  // Enable epsilon flag
                            } else {
                                // Regular transition
                                char symbol = symbolStr[0];
                                addTransition(from, symbol, to);
                            }
                            
                            transitionCount++;
                        }
                        
                        pos = endPos + 1;
                    }
                    
                }
                
                // Update counts
                numOfStates = states.size();
                numOfAlphabet = alphabets.size();
                numOfAcceptingStates = acceptingStates.size();
                
                cout << " NFA loaded - " << numOfStates << " states, " 
                    << numOfAlphabet << " symbols, " << numOfAcceptingStates 
                    << " accepting states" << endl;
                
                return true;
            } catch (...) {
                cout << " Error parsing NFA JSON file" << endl;
                return false;
            }
        }
        
        void saveToDatabase() override {
            string nfaName;
            cout << "Enter a name for this NFA: ";
            cin >> nfaName;
            
            // Create JSON file
            string jsonData = toJSON(nfaName);
            string tempFile = "temp_nfa_save.json";
            
            ofstream jsonFile(tempFile);
            if (!jsonFile.is_open()) {
                cout << "❌ Failed to create temporary JSON file!" << endl;
                return;
            }
            jsonFile << jsonData;
            jsonFile.close();
            
            // Call Python script (same as DFA)
            string command = "python db_operation.py insert " + tempFile;
            cout << " Saving NFA to database..." << endl;
            
            int result = system(command.c_str());
            
            // Cleanup temporary file
            remove(tempFile.c_str());
            
            if (result == 0) {
                cout << " NFA '" << nfaName << "' saved to database successfully!" << endl;
            } else {
                cout << " Failed to save NFA to database!" << endl;
            }
        }
        
        void loadFromDatabase(int id) override {
            string tempFile = "temp_nfa_load.json";
            string command = "python db_operation.py load " + to_string(id) + " " + tempFile;
            
            cout << " Loading NFA from database (ID: " << id << ")..." << endl;
            sleepFor(1000);
            clearScreen();
            int result = system(command.c_str());
            
            if (result == 0) {
                ifstream checkFile(tempFile);
                if (checkFile.good()) {
                    checkFile.close();
                    if (fromJSON(tempFile)) {
                        cout << " NFA loaded successfully from database!" << endl;
                        displayTransitions();
                    } else {
                        cout << " Failed to parse loaded NFA data!" << endl;
                    }
                    remove(tempFile.c_str());
                } else {
                    cout << " NFA with ID " << id << " not found in database!" << endl;
                }
            } else {
                cout << " Error loading NFA from database!" << endl;
            }
        }
        
        bool simulate(const string& input) override {
    // Start with epsilon closure of start state
    set<string> currentStates = epsilonClosure({startState});
    
    cout << " Simulating NFA with input: '" << input << "'" << endl;
    cout << " Start state(s) after epsilon closure: {";
    printStateSet(currentStates);
    cout << "}" << endl;
    
    // Process each symbol in the input
    for (size_t i = 0; i < input.length(); i++) {
        char symbol = input[i];
        
        // Check if symbol is in alphabet
        if (alphabets.find(symbol) == alphabets.end()) {
            cout << " Symbol '" << symbol << "' not in alphabet!" << endl;
            return false;
        }
        
        // Get all possible next states
        set<string> nextStates;
        for (const string& state : currentStates) {
            auto transition = transitions.find({state, symbol});
            if (transition != transitions.end()) {
                // Add all destination states
                for (const string& toState : transition->second) {
                    if (toState != "nt") {  // Skip "no transition" states
                        nextStates.insert(toState);
                    }
                }
            }
        }
        
        // Apply epsilon closure to all next states
        currentStates = epsilonClosure(nextStates);
        
        cout << " Step " << (i + 1) << ": '" << symbol << "' -> {";
        printStateSet(currentStates);
        cout << "}" << endl;
        
        // If no states reachable, reject
        if (currentStates.empty()) {
            cout << " No transitions available - REJECTED!" << endl;
            return false;
        }
    }
    
    // Check if any final state is an accepting state
    bool accepted = false;
    for (const string& state : currentStates) {
        if (acceptingStates.find(state) != acceptingStates.end()) {
            accepted = true;
            break;
        }
    }
    
    cout << " Final state(s): {";
    printStateSet(currentStates);
    cout << "}" << endl;
    cout << " Result: " << (accepted ? " ACCEPTED" : " REJECTED") << endl;
    
    return accepted;
}
void listAvailableNFA(){
    string command = "python db_operation.py listNFA";
    cout << " loading available NFA..."<< endl;
    sleepFor(2000);
    clearScreen();
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        cout << " Failed to run command!" << endl;
        return;
    }
    char buffer[128];
    string result;
    while(fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    if(result.find("EMPTY") != string::npos){
        cout << "No NFA in Database." <<endl;
        return;
    }
    try{
        size_t automataStart = result.find("\"automata\":");
        if (automataStart == string::npos) {
            cout << " Error: Could not find automata array in response" << endl;
            return ;
        }

        // Print clean header
        cout << "\n Available Finite Automata:" << endl;
        cout << string(50, '=') << endl;
        cout << left << setw(6) << "ID" << "| " 
            << setw(8) << "Type" << "| "
            << "Name" << endl;
        cout << string(50, '=') << endl;

        // Find array bounds
        size_t arrayStart = result.find('[', automataStart);
        size_t arrayEnd = result.find(']', arrayStart);
        
        if (arrayStart == string::npos || arrayEnd == string::npos) {
            cout << " Error: Invalid JSON format" << endl;
            return ;
        }
        string jsonArray = result.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
        size_t pos = 0;
        
        while ((pos = jsonArray.find('{', pos)) != string::npos) {
            size_t endPos = jsonArray.find('}', pos);
            if (endPos == string::npos) break;

            string item = jsonArray.substr(pos, endPos - pos + 1);
            
            string idStr = extractField(item, "id");
            string type = extractField(item, "type");
            string name = extractField(item, "name");
            // Print clean formatted row
            cout << left << setw(6) << idStr << "| "
                 << setw(8) << type << "| "
                 << name << endl;

            pos = endPos + 1;
        }
        cout << string(50, '=') << endl;
    }catch(const exception& e) {
        cout << " Error parsing NFA list: " << e.what() << endl;
    }
}
// Helper function to compute epsilon closure
set<string> epsilonClosure(const set<string>& states) {
    cout << "DEBUG: Computing epsilon closure for: {";
    for (const string& s : states) cout << s << " ";
    cout << "}" << endl;
    
    set<string> closure = states; // Start with input states
    bool changed = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 100;
    
    cout << "DEBUG: Initial closure: {";
    for (const string& s : closure) cout << s << " ";
    cout << "}" << endl;
    
    while (changed && iterations < MAX_ITERATIONS) {
        changed = false;
        set<string> newStates = closure;
        iterations++;
        
        cout << "DEBUG: Iteration " << iterations << endl;
        
        for (const string& state : closure) {
            if (isAllowEpsilonTransitions) {
                cout << "DEBUG: Checking epsilon transitions from " << state << endl;
                
                // Check for epsilon transitions
                auto epsilonTrans = transitions.find({state, 'e'});
                if (epsilonTrans != transitions.end()) {
                    cout << "DEBUG: Found epsilon transitions from " << state << ": ";
                    for (const string& epsilonState : epsilonTrans->second) {
                        cout << epsilonState << " ";
                        if (epsilonState != "nt" && newStates.find(epsilonState) == newStates.end()) {
                            newStates.insert(epsilonState);
                            changed = true;
                            cout << "(ADDED) ";
                        }
                    }
                    cout << endl;
                } else {
                    cout << "DEBUG: No epsilon transitions from " << state << endl;
                }
            }
        }
        
        closure = newStates;
        cout << "DEBUG: Closure after iteration " << iterations << ": {";
        for (const string& s : closure) cout << s << " ";
        cout << "}" << endl;
    }
    
    cout << "DEBUG: Final epsilon closure: {";
    for (const string& s : closure) cout << s << " ";
    cout << "}" << endl;
    
    return closure;
}

// Helper function to print state sets nicely
void printStateSet(const set<string>& states) {
    bool first = true;
    for (const string& state : states) {
        if (state == "nt") continue; // Skip "no transition" state
        if (!first) cout << ", ";
        cout << state;
        first = false;
    }
    if (states.empty() || (states.size() == 1 && states.count("nt"))) {
        cout << "∅"; // Empty set symbol
    }
}
        
        void addTransition(const string& from, char symbol, const string& to) {
            transitions[{from, symbol}].insert(to);
        }
        void displayTransitions() override{
            cout << "\n transition Table" << endl;
            cout << "-------------------" << endl;
            for (const auto& transition : transitions) {
                const string& fromState = transition.first.first;
                char symbol = transition.first.second;
                const set<string>& toStates = transition.second;
                
                // Display all transitions for this state-symbol pair
                for (const string& toState : toStates) {
                    if(fromState == "nt"){
                        break;
                    }else{
                        if(fromState == startState){
                            cout << fromState + "*" << " --" << symbol << "--> " << toState << endl;
                        }else{
                            cout << fromState << " --" << symbol << "--> " << toState << endl;
                        }
                    }
                }
            }
        }
        void handleInputForNFA(){
            cout << "======== Designing NFA ========="<<endl;
            bool isValid = false;
            do{
                cout << "Is this NFA have epsilon transitions?(y/n): ";
                char choice;
                cin >> choice;
                if(choice == 'y' || choice == 'n'){
                    isValid = true;
                    isAllowEpsilonTransitions = choice == 'y' ? true : false;
                }else{
                    cout << "Error: Invalid choice. Please enter y or n."<<endl;
                }
            }while(!isValid);
            int numStates;
            do{
                cout << "Enter number of states : ";
                cin >> numStates;
                if(numStates <= 0){
                    cout << "Error: Number of states must be greater than 0."<< endl;
                }
            }while(numStates <= 0);
            numOfStates = numStates;
            string notransition = "nt";
            addStates(notransition);
            for(int i = 0 ; i < numOfStates ; i++){
                string state = "q" + to_string(i);
                addStates(state);
            }
            int numAlphabet;
            do{
                cout << "Enter number of symbols : ";
                cin >> numAlphabet;
                if(numAlphabet <= 0){
                    cout << "Error: Number of symbols must be greater than 0." << endl;
                }
            }while(numAlphabet <=0 );
            numOfAlphabet = numAlphabet;
            for(int i = 0 ; i < numOfAlphabet ; i++){
                char symbol;
                cout << "Enter symbol " << i+1 << ": ";
                cin >> symbol;
                addSymbol(symbol);
            }
            sleepFor(1000);
            clearScreen();
            cout << "You have " << numOfStates << " states there're ";
            displayState();
            cout << " and " << numOfAlphabet << " symbols there're ";
            displaySymbol();
            cout << "in your NFA." << endl;
            string state_state;
            do{
                cout << "Enter start state : ";
                cin >> state_state;
                if(states.find(state_state)== states.end()){
                    cout << "Error: Start state must be one of the defined states." << endl;
                }
            }while(states.find(state_state) == states.end());
            startState = state_state;
            int numOfAcc;
            do{
                cout << "Enter number of accepting states : ";
                cin >> numOfAcc;
                if(numOfAcc <= 0){
                    cout << "Error: Number of accepting states must be greater than 0."<< endl;
                }
            }while(numOfAcc <= 0);
            numOfAcceptingStates = numOfAcc;
            for(int i = 0 ;i < numOfAcceptingStates ; i++){
                string acceptingState;
                do{
                    cout << "Enter accepting state " << i+1 << ": ";
                    cin >> acceptingState;
                    if(states.find(acceptingState) == states.end()){
                        cout << "Error: Accepting state must be one of the defined states." << endl;
                    }
                }while(states.find(acceptingState) == states.end());
                addAcceptingStates(acceptingState);
            }
            for(const auto& state : states){
                for(const auto& alphabet : alphabets){
                    char addmore;
                    do{
                        string toState;
                        do{
                            if(state =="nt"){
                                break;
                            }
                            cout << "Enter transition from state "<<state << " with symbol "<<alphabet << " to state (if no transition enter \"nt\"): ";
                            cin >> toState;
                            if(states.find(toState)==states.end()){
                                cout << "Error: Transition state must be one of the defined states."<<endl;
                            }
                        }while(states.find(toState) == states.end());
                        addTransition(state, alphabet, toState);
                        if(state != "nt"){
                            cout << "is there any more transition from state " << state << " with symbol " << alphabet << "? (y/n): ";
                        cin >> addmore;
                        }
                    }while(addmore=='y');
                }
            }
            sleepFor(1000);
            clearScreen();
            if(isAllowEpsilonTransitions) {
                addSymbol('e');
                cout << "\n--- Adding Epsilon Transitions (ep) ---" << endl;
                for(const auto& state : states){
                    if(state == "nt") continue;
                    
                    char hasEpsilon;
                    cout << "Does state " << state << " have epsilon transitions? (y/n): ";
                    cin >> hasEpsilon;
                    
                    if(hasEpsilon == 'y' || hasEpsilon == 'Y') {
                        char addMore = 'y';
                        while(addMore == 'y') {
                            string toState;
                            do{
                                cout << "Enter epsilon transition from " << state << " to state: ";
                                cin >> toState;
                                if(states.find(toState) == states.end()){
                                    cout << "Error: Transition state must be one of the defined states." << endl;
                                    displayState();
                                    cout << endl;
                                }
                            }while(states.find(toState) == states.end());
                            
                            addTransition(state, 'e', toState);
                            cout << "✅ Added epsilon transition: " << state << " --epsilon--> " << toState << endl;
                            
                            cout << "Add another epsilon transition from " << state << "? (y/n): ";
                            cin >> addMore;
                        }
                    }
                }
            }
            cout << "\n NFA created successfully!" << endl;
            displayTransitions();
            pauseAndClear();
            cout << "\n Do you want to test this NFA? (y/n): ";
            char testChoice;
            cin >> testChoice;
            if(testChoice == 'y'){
                string testInput;
                do {
                    cout << "Enter a string to test (or 'quit' to stop): ";
                    cin >> testInput;
                    if(testInput != "quit") {
                        cout << "\n" << string(30, '-') << endl;
                        simulate(testInput);
                        cout << string(30, '-') << endl;
                    }
                } while(testInput != "quit");
            }
            pauseAndClear();
            char saveChoice;
            cout << "\n Do you want to save this NFA to database? (y/n): ";
            cin >> saveChoice;
            if(saveChoice == 'y' || saveChoice == 'Y') {
                saveToDatabase();
            }
        }
};

void menu() {
    cout << "\n" << string(40, '=') << endl;
    cout << "     FINITE AUTOMATON SIMULATOR" << endl;
    cout << string(40, '=') << endl;
    cout << " 1. Design a FA" << endl;
    cout << " 2. Simulate a FA from Database" << endl;
    cout << " 3. Convert NFA to DFA" << endl;
    cout << " 4. Check type of FA" << endl;
    cout << " 5. Minimize DFA" << endl;
    cout << " 0. Exit" << endl;
    cout << string(40, '=') << endl;
    cout << "Please enter your choice: ";
}

void designMenu() {
    clearScreen();
    cout << "\n" << string(40, '-') << endl;
    cout << "        Design Finite Automaton" << endl;
    cout << string(40, '-') << endl;
    cout << " 1. Create DFA" << endl;
    cout << " 2. Create NFA" << endl;
    cout << " 0. Back to main menu" << endl;
    cout << "Please enter your choice: ";
}

void checkTypeMenu(){
    cout << "===== Check FA type from Database ======"<<endl;
    DFA temp;
    map<int,string> faTypes = temp.listAvailableFA();

    if(faTypes.empty()){
        cout << " No Finite Automata available in the database!" << endl;
        return;
    }
    cout << "Enter FA ID to load: ";
    int id;
    cin >> id;

    if(faTypes.find(id)==faTypes.end()){
        cout << " Invalid FA ID! Please choose from the available IDs." << endl;
        return;
    }

    string fatype = faTypes[id];
    FiniteAutoMaton* fa = nullptr;

    // Load as NFA first (since NFA can handle both types)
    fa = new NFA();
    cout << " Loading FA for type analysis..." << endl;
    
    fa->loadFromDatabase(id);
    
    // Cast to NFA for detailed analysis
    NFA* nfa = static_cast<NFA*>(fa);
    
    cout << "\n Analyzing FA Structure..." << endl;
    sleepFor(2000);
    cout << "================================" << endl;
    
    // Get NFA transitions for analysis
    auto& nfaTransitions = nfa->getNFATransitions();
    
    bool hasEpsilonTransitions = false;
    bool hasMultipleTransitions = false;
    bool hasMissingTransitions = false;
    
    // Step 1: Check for epsilon transitions
    cout << " Step 1: Checking for epsilon transitions..." << endl;
    sleepFor(1500);
    for(const auto& transition : nfaTransitions) {
        if(transition.first.second == 'e') {
            hasEpsilonTransitions = true;
            cout << "    Found epsilon transition: " 
                 << transition.first.first << " --e--> ";
            for(const string& to : transition.second) {
                cout << to << " ";
            }
            cout << endl;
        }
    }
    
    if(hasEpsilonTransitions) {
        cout << " RESULT: This is an NFA (has epsilon transitions)" << endl;
        delete fa;
        pauseAndClear();
        return;
    }
    cout << "    No epsilon transitions found" << endl;
    
    // Step 2: Check for multiple transitions from same state with same symbol
    cout << "\n Step 2: Checking for nondeterministic transitions..." << endl;
    sleepFor(1500);
    for(const auto& transition : nfaTransitions) {
        if(transition.second.size() > 1) {
            hasMultipleTransitions = true;
            cout << "    Found multiple transitions: " 
                 << transition.first.first << " --" << transition.first.second << "--> {";
            bool first = true;
            for(const string& to : transition.second) {
                if(!first) cout << ", ";
                cout << to;
                first = false;
            }
            cout << "} (" << transition.second.size() << " destinations)" << endl;
        }
    }
    
    if(hasMultipleTransitions) {
        cout << " RESULT: This is an NFA (has nondeterministic transitions)" << endl;
        delete fa;
        pauseAndClear();
        return;
    }
    cout << "    No multiple transitions found" << endl;
    
    // Step 3: Check for missing transitions (incomplete DFA)
    cout << "\n Step 3: Checking for missing transitions..." << endl;
    sleepFor(1500);
    set<string> states = fa->getStates();
    set<char> alphabet = fa->getAlphabet();
    
    // Remove "nt" from states for analysis
    states.erase("nt");
    
    int expectedTransitions = states.size() * alphabet.size();
    int actualTransitions = 0;
    
    for(const string& state : states) {
        for(char symbol : alphabet) {
            if(symbol == 'e') continue; // Skip epsilon in alphabet check
            
            auto transition = nfaTransitions.find({state, symbol});
            if(transition != nfaTransitions.end() && !transition->second.empty()) {
                // Check if it's a real transition (not "nt")
                bool hasRealTransition = false;
                for(const string& to : transition->second) {
                    if(to != "nt") {
                        hasRealTransition = true;
                        break;
                    }
                }
                if(hasRealTransition) {
                    actualTransitions++;
                } else {
                    hasMissingTransitions = true;
                    cout << "     Missing transition: δ(" << state << ", " << symbol << ") = undefined" << endl;
                }
            } else {
                hasMissingTransitions = true;
                cout << "     Missing transition: δ(" << state << ", " << symbol << ") = undefined" << endl;
            }
        }
    }
    
    if(hasMissingTransitions) {
        cout << " RESULT: This is an NFA (incomplete - has missing transitions)" << endl;
        cout << "   Expected transitions: " << expectedTransitions << endl;
        cout << "   Actual transitions: " << actualTransitions << endl;
        delete fa;
        pauseAndClear();
        return;
    }
    cout << "    All transitions are defined" << endl;
    
    // Step 4: Final determination
    cout << "\n Step 4: Final analysis..." << endl;
    sleepFor(1500);
    cout << "    No epsilon transitions" << endl;
    cout << "    No multiple transitions per state-symbol pair" << endl;
    cout << "    All transitions are defined" << endl;
    cout << "    Exactly one transition per state-symbol pair" << endl;
    
    cout << "\n RESULT: This is a DFA (Deterministic Finite Automaton)" << endl;
    
    // Display comprehensive analysis
    cout << "\n Detailed Analysis:" << endl;
    cout << "================================" << endl;
    cout << "Type: DFA" << endl;
    cout << "States: " << states.size() << endl;
    cout << "Alphabet symbols: " << alphabet.size() << endl;
    cout << "Total transitions: " << actualTransitions << endl;
    cout << "Completeness: Complete" << endl;
    cout << "Determinism: Deterministic" << endl;
    pauseAndClear();
    delete fa;
}
void simulateMenu() {
    cout << "\n=== Simulate a FA from Database ===" << endl;
    
    DFA tempDFA;
    map<int,string> faTypes = tempDFA.listAvailableFA();

    if(faTypes.empty()){
        cout << " No Finite Automata available in the database!" << endl;
        return;
    }
    cout << "\nEnter FA ID to load: ";
    int id;
    cin >> id;

    if (faTypes.find(id) == faTypes.end()) {
        cout << " Invalid FA ID! Please choose from the available IDs." << endl;
        return;
    }

    string faType = faTypes[id];
    FiniteAutoMaton* fa = nullptr;

    if (faType == "NFA") {
        fa = new NFA();
    } else {
        fa = new DFA();
    }
    clearScreen();
    fa->loadFromDatabase(id);

    
    // Test the loaded FA
    char testChoice;
    cout << "\n Do you want to test this FA? (y/n): ";
    cin >> testChoice;
    
    if(testChoice == 'y' || testChoice == 'Y') {
        string testInput;
        do {
            cout << "Enter a string to test (or 'quit' to stop): ";
            cin >> testInput;
            if(testInput != "quit") {
                cout << "\n" << string(30, '-') << endl;
                if(fa->simulate(testInput)) {
                    cout << " ACCEPTED!" << endl;
                } else {
                    cout << " REJECTED!" << endl;
                }
                cout << string(30, '-') << endl;
            }
        } while(testInput != "quit");
    }
    
    delete fa;
    pauseAndClear();
}



void handleUserInputForMenu() {
    int choice;
    do {
        menu();
        cin >> choice;
        switch(choice) {
            case 1: {
                int designChoice;
                do {
                    designMenu();
                    cin >> designChoice;
                    switch(designChoice) {
                        case 1: {
                            clearScreen();
                            DFA dfa;
                            dfa.handleInputForDFA();
                            pauseAndClear();
                            break;
                        }
                        case 2: {
                            clearScreen();
                            NFA nfa;
                            nfa.handleInputForNFA();
                            pauseAndClear();
                            break;
                        }
                        case 0:
                            cout << "Returning to main menu..." << endl;
                            sleepFor(1000);
                            clearScreen();
                            break;
                        default:
                            cout << " Invalid choice! Please try again." << endl;
                    }
                } while(designChoice != 0);
                break;
            }
            case 2: {
                sleepFor(300);
                clearScreen();
                simulateMenu();
                break;
            }
            case 3: {
                sleepFor(300);
                clearScreen();
                cout << "========= Convert NFA to DFA =========" << endl;
                NFA nfa;
                nfa.listAvailableNFA();
                cout << "Enter NFA's ID to convert: ";
                int nfaId;
                cin >> nfaId;
                nfa.loadFromDatabase(nfaId);
                string jsonData = nfa.toJSON("converted_dfa.json");
                string tempFile = "nfa_input.json";
                ofstream jsonFile(tempFile);
                if(!jsonFile.is_open()){
                    cout << " Failed to create temporary JSON file!" << endl;
                    return;
                }
                jsonFile << jsonData;
                jsonFile.close();
                string command = "python nfa_to_dfa.py";
                cout << " Converting NFA to DFA..." << endl;
                int result = system(command.c_str());
                if(result == 0) {
                    cout << " NFA converted to DFA successfully!" << endl;
                    DFA dfa;
                    dfa.fromJSON("dfa_output.json");
                    remove(tempFile.c_str());
                    remove("dfa_output.json");
                    cout << "do you want to save this DFA to database? (y/n): ";
                    char saveChoice;
                    cin >> saveChoice;
                    if(saveChoice == 'y' || saveChoice == 'Y') {
                        dfa.saveToDatabase();
                    }
                } else {
                    cout << " Failed to convert NFA to DFA!" << endl;
                }
                break;
            }
            case 4: {
                sleepFor(500);
                clearScreen();
                checkTypeMenu();
                break;
            }
            case 5: {
                cout << " Minimize DFA - Feature coming soon!" << endl;
                break;
            }
            case 0: {
                cout << " Thank you for using FA Simulator! Goodbye!" << endl;
                break;
            }
            default: {
                cout << " Invalid choice! Please enter a number between 0-5." << endl;
            }
        }
    } while(choice != 0);
}

// Main function
int main() {
    handleUserInputForMenu();
    return 0;
}