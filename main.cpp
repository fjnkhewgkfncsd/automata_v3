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
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

void clearScreen() {
    #ifdef _WIN32
        system("cls");     // Windows
    #else
        system("clear");   // Unix/Linux/macOS
    #endif
}

void pauseAndClear() {
    cout << "\n\033[38;5;196mPress Enter to continue...\033[0m";
    cin.sync(); // flush input buffer (works on most platforms)
    cin.get();
    clearScreen();
}

int getValidatedInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.fail()) {
            cin.clear(); // clear error flags
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard invalid input
            cout << "\033[1;31mInvalid input! Please enter a number.\033[0m" << endl;
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard any extra input
            return value;
        }
    }
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
                    cout <<"\033[38;5;220m"<< state +"* "<<"\033[0m";
                }else{
                    cout <<"\033[38;5;220m"<< state <<" \033[0m";
                }
            }
        }
        virtual void displaySymbol(){
            for(const char symbol : alphabets){
                cout <<"\033[38;5;220m"<< symbol << "\033[0m" << " ";
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
        virtual void displayStartState() const {
            cout << "\033[1;32mStart State: \033[0m\033[38;5;220m" << startState << "\033[0m" << endl;
        }
        virtual int getNumOfAcceptingState(){
            return numOfAcceptingStates;
        }
        map<int,string> listAvailableFA(){
            string command = "python db_operation.py list";
            cout << "\033[1;32m Loading available finite automata...\033[0m" << endl;
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
                cout << "\n\033[1;36m          Available Finite Automata\033[0m" << endl;
                cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;
                cout << "\033[35m|\033[0m \033[1;31mID   \033[0m| \033[1;31mType   \033[0m| \033[1;31mName\033[0m" << string(50 - 30, ' ') << "\033[35m|\033[0m\n";
                cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;

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
                    cout << "\033[35m|\033[0m" // left border in magenta
                    << "\033[32m" << setw(6) << left << idStr << "\033[0m"
                    << "\033[35m|\033[0m"
                    << "\033[33m" << setw(8) << left << type << "\033[0m"
                    << "\033[35m|\033[0m"
                    << "\033[34m" << setw(25) << left << name << "\033[0m"
                    << "\033[35m|\033[0m"
                    << endl;

                    pos = endPos + 1;
                }
                cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;
                
            } catch (const exception& e) {
                cout << "\033[38;5;220m Error parsing response: " << e.what() << "\033[0m" << endl;
            } catch (...) {
                cout << "\033[38;5;220m Unknown error parsing response\033[0m" << endl;
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
        int stateCounter = 0;
        map<set<string>, string> dfaStateName;
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

        set<int> listAvailableDFA(){
            string command = "python db_operation.py listDFA";
            cout << "\033[38;2;255;105;180mLoading available DFA...\033[0m" << endl;
            sleepFor(2000);
            clearScreen();
            set<int> dfaIds;
            FILE* pipe = popen(command.c_str(), "r");
            if(!pipe){
                cout << "\033[38;5;220mFailed to execute Python script!\033[0m" << endl;
                return dfaIds;
            }
            char buffer[1024];
            string result;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            pclose(pipe);
            if (result.find("EMPTY") != string::npos) {
                cout << "\033[38;5;220mNo DFA found in database.\033[0m" << endl;
                return dfaIds;
            }
            try {
                size_t automataStart = result.find("\"automata\":");
                if (automataStart == string::npos) {
                    cout << "\033[38;5;220m Error: Could not find automata array in response\033[0m" << endl;
                    return dfaIds;
                }

                // Print clean header
                cout << "\n\033[1;36m Available DFA:\033[0m" << endl;
                cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;
                cout << "\033[35m|\033[0m \033[1;31mID   \033[0m| \033[1;31mType   \033[0m| \033[1;31mName\033[0m" << string(50 - 30, ' ') << "\033[35m|\033[0m\n";
                cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;

                // Find array bounds
                size_t arrayStart = result.find('[', automataStart);
                size_t arrayEnd = result.find(']', arrayStart);
                
                if (arrayStart == string::npos || arrayEnd == string::npos) {
                    cout << " Error: Invalid JSON format" << endl;
                    return dfaIds;
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
                    dfaIds.insert(stoi(idStr));
                    // Print clean formatted row
                    cout << "\033[35m|\033[0m" // left border in magenta
                    << "\033[32m" << setw(6) << left << idStr << "\033[0m"
                    << "\033[35m|\033[0m"
                    << "\033[33m" << setw(8) << left << type << "\033[0m"
                    << "\033[35m|\033[0m"
                    << "\033[34m" << setw(25) << left << name << "\033[0m"
                    << "\033[35m|\033[0m"
                    << endl;

                    pos = endPos + 1;
                }
                cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;
                
            } catch (const exception& e) {
                cout << "\033[38;5;220m Error parsing response: " << e.what() << "\033[0m" << endl;
            } catch (...) {
                cout << "\033[38;5;220m Unknown error parsing response\033[0m" << endl;
            }
            return dfaIds;
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
        void displayAcceptingStates() const {
            for(const auto& state : acceptingStates){
                cout << "\033[38;5;220m" << state << "\033[0m" << " ";
            }
        }

        void getDetails(){
            cout << "\033[1;32mStates : \033[0m";
            displayState();
            cout << endl;
            displayStartState();
            cout << "\033[1;32mSymbols : \033[0m";
            displaySymbol();
            cout << endl;
            cout << "\033[1;32mAccepting states : \033[0m";
            displayAcceptingStates();
            displayTransitions();
        }
        void saveToDatabase() override {
            string dfaName;
            cout << "\033[38;5;202mEnter a name for this DFA: \033[0m";
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
            cout << "\033[1;32m Saving DFA to database...\033[0m" << endl;

            // Use simple system() call instead of popen
            int result = system(command.c_str());
            
            // Cleanup temporary file
            remove(tempFile.c_str());
            
            // Just check the result
            if (result == 0) {
                cout << "\033[1;32m DFA '" << dfaName << "' saved to database successfully!\033[0m" << endl;
            } else {
                cout << "\033[1;31m Failed to save DFA to database!\033[0m" << endl;
            }
        }
        
        void loadFromDatabase(int id) override {
            string tempFile = "temp_dfa_load.json";
            string command = "python db_operation.py load " + to_string(id) + " " + tempFile;
            
            cout << "\033[38;2;255;105;180mLoading DFA from database (ID: " << id << ")...\033[0m" << endl;
            
            int result = system(command.c_str());
            
            if (result == 0) {
                // Check if file was created successfully
                ifstream checkFile(tempFile);
                if (checkFile.good()) {
                    checkFile.close();
                    if (fromJSON(tempFile)) {
                        cout << "\033[92m DFA loaded successfully from database!\033[0m" << endl;
                        cout << "\033[93m Notice: The start state is marked with an asterisk (*)\033[0m" << endl;
                        cout << "\033[1;36m DFA Details:\033[0m" << endl;
                        getDetails();
                    } else {
                        cout <<  "\033[1;31m Failed to parse loaded DFA data!\033[0m" << endl;
                    }
                    
                    remove(tempFile.c_str());
                } else {
                    cout << "\033[1;31m DFA with ID " << id << " not found in database!\033[0m" << endl;
                }
            } else {
                cout << "\033[1;31m Error loading DFA from database!\033[0m" << endl;
            }
        }
        
        bool simulate(const string& input) override {
            string currentState = startState;
            cout << "\033[38;5;117m Simulating input: \033[0m'" <<"\033[38;5;220m"<< input <<"\033[0m" << "'" << endl;
            cout << "\033[38;5;117m  Start state: \033[0m" << "\033[38;5;220m"<<currentState << "\033[0m" << endl;

            for (char symbol : input) {
                if (alphabets.find(symbol) == alphabets.end()) {
                    cout << "\033[1;31m Symbol '" << symbol << "' not in alphabet!\033[0m" << endl;
                    return false;
                }
                
                auto transition = transitions.find({currentState, symbol});
                if (transition == transitions.end()) {
                    cout << "\033[1;31m No transition from " << currentState << " with symbol " << symbol << "\033[0m" << endl;
                    return false;
                }
                
                cout << "   \033[1;32m" << currentState<<"\033[0m" <<"\033[1;33m"<< " --" << symbol << "--> " <<"\033[0m"<<"\033[1;31m"<< transition->second <<"\033[0m"<< endl;
                currentState = transition->second;
            }
            
            bool accepted = acceptingStates.find(currentState) != acceptingStates.end();
            cout << "\033[38;5;117m Final state: \033[0m \033[38;5;220m" << currentState<< "\033[0m";
            return accepted;
        }
        
        void addTransition(const string& from, char symbol, const string& to) {
            transitions[{from, symbol}] = to;
        }
        void displayTransitions() override {
            cout << "\n\033[1;36m Transition Table:\033[0m" << endl;
            cout << "\033[1;35m-------------------\033[0m" << endl;
            for (const auto& transition : transitions) {
                if(transition.first.first ==startState ){
                    cout <<"\033[1;32m" <<transition.first.first + "*"<<"\033[0m" <<"\033[1;33m"<< " --" << transition.first.second 
                        << "--> "<<"\033[0m" <<"\033[1;31m"<< transition.second << "\033[0m" << endl;
                }else{
                    cout <<"\033[1;32m" <<transition.first.first <<"\033[0m" <<"\033[1;33m"<< " --" << transition.first.second 
                        << "--> "<<"\033[0m" <<"\033[1;31m"<< transition.second << "\033[0m" << endl;
                }
            }
            cout << endl;
        }
        // Getters for database operations
        map<pair<string,char>, string>& getTransitions(){ return transitions; }
        const string& getStartState() const { return startState; }
        const set<string>& getAcceptingStates() const { return acceptingStates; }
            
        set<string> jsonArrayTostate(const json& jarr){
            set<string> result;
            if (jarr.is_array()) {
                for(const auto& item : jarr){
                    if(item.is_string())
                        result.insert(item.get<string>());
                }
            } else if (jarr.is_string()) {
                result.insert(jarr.get<string>());
            }
            return result;
        }
        
        string getDFAStateName(const set<string>& s) {
            if (dfaStateName.find(s) == dfaStateName.end()) {
                dfaStateName[s] = "q" + to_string(stateCounter++);
            }
            return dfaStateName[s];
        }
        void extractFromJson(const json& j){
            states.clear();
            acceptingStates.clear();
            transitions.clear();
            dfaStateName.clear();
            stateCounter = 0;
            
            for(const auto& stateGroup : j["states"]){
                set<string> s = flattenStateJson(stateGroup);
                string stateName = getDFAStateName(s);
                states.insert(stateName);
            }

            if (j.contains("startState") || j.contains("startStart")) {
                const auto& startJson = j.contains("startState") ? j["startState"] : j["startStart"];
                set<string> startset = flattenStateJson(startJson);
                startState = getDFAStateName(startset);
                states.insert(startState);
            }

            for(const auto& acc : j["acceptingStates"]){
                set<string> s = flattenStateJson(acc);
                string accName = getDFAStateName(s);
                acceptingStates.insert(accName);
                states.insert(accName);
            }

            for (const auto& trans : j["transitions"]) {
                set<string> fromState = flattenStateJson(trans["from"]);
                char symbol = trans["symbol"].get<string>()[0];
                set<string> toState = flattenStateJson(trans["to"]);

                string fromName = getDFAStateName(fromState);
                string toName = getDFAStateName(toState);

                transitions[{fromName, symbol}] = toName;
                states.insert(fromName);
                states.insert(toName);
            }
            alphabets.clear();
            if (j.contains("alphabet")) {
                for (const auto& symbol : j["alphabet"]) {
                    if (!symbol.is_null() && !symbol.get<string>().empty())
                        alphabets.insert(symbol.get<string>()[0]);
                }
            } else if (j.contains("transitions")) {
                // Extract unique symbols from transitions
                for (const auto& trans : j["transitions"]) {
                    if (trans.contains("symbol")) {
                        string symbolStr = trans["symbol"].get<string>();
                        if (!symbolStr.empty())
                            alphabets.insert(symbolStr[0]);
                    }
                }
            }

            // 6. Update counts
            numOfStates = states.size();
            numOfAlphabet = alphabets.size();
            numOfAcceptingStates = acceptingStates.size();
        }
        set<string> flattenStateJson(const json& j) {
            set<string> result;
            if (j.is_string()) {
                result.insert(j.get<string>());
            } else if (j.is_array()) {
                for (const auto& elem : j) {
                    set<string> sub = flattenStateJson(elem);
                    result.insert(sub.begin(), sub.end());
                }
            }
            return result;
        }
        void handleInputForDFA(){
            cout << "\033[1;36m======== Designing DFA =========\033[0m" << endl;
            int numState;
            do{
                numState = getValidatedInt("\033[38;5;202mEnter number of states : \033[0m");
                if(numState < 1){
                    cout << "\033[38;5;226mError: Invalid number of states.\033[0m" << endl;
                }
            } while(numState < 1);
            numOfStates = numState;
            for(int i = 0 ; i < numOfStates ; i++){
                string state = "q"+ to_string(i);
                addStates(state);
            }
            int numAlphabet;
            do{
                numAlphabet = getValidatedInt("\033[38;5;202mEnter number of symbols in alphabet : \033[0m");
                if(numAlphabet < 1){
                    cout << "\033[38;5;226mError: Invalid number of symbols.\033[0m" << endl;
                }
            }while(numAlphabet < 1);
            numOfAlphabet = numAlphabet;
            for(int i = 0 ; i < numOfAlphabet ; i++){
                char symbol;
                cout << "\033[38;5;202mEnter symbol "<< i+1 << ": \033[0m";
                cin >> symbol;
                addSymbol(symbol);
            } 
            cout << "\033[1;32mYou have " <<numOfStates << " states there're ";
            displayState(); 
            cout <<"\033[1;32m and "<< numOfAlphabet << " symbols there're \033[0m";
            displaySymbol();
            cout <<"\033[1;32min your DFA.\033[0m"<<endl;
            string startstate;
            do{
                cout << "\033[38;5;202mEnter start state : \033[0m";
                cin >> startstate;
                if(states.find(startstate) == states.end()){
                    cout << "\033[38;5;226mError : Start state must be one of the defined states.\033[0m"<<endl;
                }
            }while(states.find(startstate) == states.end());
            startState = startstate;
            int acceptingState;
            do{
                acceptingState = getValidatedInt("\033[38;5;202mEnter number of accepting states : \033[0m");
                if(acceptingState < 1){
                    cout << "\033[38;5;226mError: Invalid number of accepting states.\033[0m" << endl;
                }
            }while(acceptingState < 1);
            numOfAcceptingStates = acceptingState ;
            for(int i =0 ; i < numOfAcceptingStates ; i++){
                string acceptingState;
                do{
                    cout << "\033[38;5;202mEnter accepting states " << i+1 << " : \033[0m";
                    cin >> acceptingState;
                    if(states.find(acceptingState) == states.end()){
                        cout << "\033[38;5;226mError: Accepting state must be one of the defined states.\033[0m" << endl;
                    } 
                }while(states.find(acceptingState) == states.end());  
                addAcceptingStates(acceptingState);
            }
            for(const auto& state : states){
                for(const auto& alphabet : alphabets){
                    string toState;
                    do{
                        cout << "\033[38;5;202mEnter transition from state "<<state << " with symbol "<<alphabet << " to state : \033[0m";
                        cin >> toState;
                        if(states.find(toState)==states.end()){
                            cout << "\033[38;5;226mError: Transition state must be one of the defined states.\033[0m"<<endl;
                        }
                    }while(states.find(toState) == states.end());
                    addTransition(state, alphabet, toState);
                }
            }
            
            // ✅ ADDED: Test the DFA and offer to save to database
            cout << "\n\033[1;32m DFA created successfully!\033[0m" << endl;
            displayTransitions();
            pauseAndClear();
            // Test the DFA
            char testChoice;
            bool isacc;
            do{
                cout << "\n\033[38;5;202m Do you want to test the DFA? (y/n): \033[0m";
                cin >> testChoice;
                if(testChoice !='y' && testChoice != 'n'){
                    cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                    isacc = true;
                }else{
                    isacc = false;
                }
            }while(isacc);
            
            if(testChoice == 'y' || testChoice == 'Y') {
                string testInput;
                do {
                    cout << "\033[38;5;202mEnter a string to test (or 'quit' to stop): \033[0m";
                    cin >> testInput;
                    if(testInput != "quit") {
                        cout << "\n" << string(30, '-') << endl;
                        if(simulate(testInput)) {
                            cout << "\033[32m ACCEPTED!\033[0m" << endl;
                        } else {
                            cout << "\033[31m REJECTED!\033[0m" << endl;
                        }
                        cout << string(30, '-') << endl;
                    }
                } while(testInput != "quit");
            }
                sleepFor(500);
                pauseAndClear();
            // Save to database
            char saveChoice;
            do{
                cout << "\n\033[38;5;202mDo you want to save this NFA to database? (y/n): \033[0m";
                cin >> saveChoice;
                if(saveChoice !='y' && saveChoice !='n'){
                    cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                    isacc = true;
                }else{
                    isacc = false;
                }
            }while(isacc);
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

                cout << "\033[1;32m NFA loaded - " << numOfStates << " states, "
                    << numOfAlphabet << " symbols, " << numOfAcceptingStates
                    << " accepting states\033[0m" << endl;

                return true;
            } catch (...) {
                cout << " Error parsing NFA JSON file" << endl;
                return false;
            }
        }
        
        void saveToDatabase() override {
            string nfaName;
            cout << "\033[38;5;202mEnter a name for this NFA: \033[0m";
            cin >> nfaName;
            
            // Create JSON file
            string jsonData = toJSON(nfaName);
            string tempFile = "temp_nfa_save.json";
            
            ofstream jsonFile(tempFile);
            if (!jsonFile.is_open()) {
                cout << " Failed to create temporary JSON file!" << endl;
                return;
            }
            jsonFile << jsonData;
            jsonFile.close();
            
            // Call Python script (same as DFA)
            string command = "python db_operation.py insert " + tempFile;
            cout << "\033[1;32m Saving NFA to database...\033[0m" << endl;

            int result = system(command.c_str());
            
            // Cleanup temporary file
            remove(tempFile.c_str());
            
            if (result == 0) {
                cout << "\033[1;32m NFA '" << nfaName << "' saved to database successfully!\033[0m" << endl;
            } else {
                cout << "\033[38;5;196m Failed to save NFA to database!\033[0m" << endl;
            }
        }
        
        void loadFromDatabase(int id) override {
            string tempFile = "temp_nfa_load.json";
            string command = "python db_operation.py load " + to_string(id) + " " + tempFile;
            
            cout << "\033[38;2;255;105;180m Loading NFA from database (ID: " << id << ")...\033[0m" << endl;
            sleepFor(1000);
            clearScreen();
            int result = system(command.c_str());
            
            if (result == 0) {
                ifstream checkFile(tempFile);
                if (checkFile.good()) {
                    checkFile.close();
                    if (fromJSON(tempFile)) {
                        cout << "\033[92m NFA loaded successfully from database!\033[0m" << endl;
                        cout << "\033[93m Notice: The start state is marked with an asterisk (*)\033[0m" << endl;
                        cout << "\033[1;36m NFA Details:\033[0m" << endl;
                        getDetails();
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
        void getDetails() {
            cout << "\033[1;32mStates : \033[0m";
            displayState();
            cout << endl;
            displayStartState();
            cout << "\033[1;32mSymbols : \033[0m";
            displaySymbol();
            cout << endl;
            cout << "\033[1;32mAccepting states : \033[0m";
            displayAcceptingStates();
            displayTransitions();
        }
        void displayAcceptingStates() const {
            for(const auto& state : acceptingStates){
                cout << "\033[38;5;220m" << state << "\033[0m" << " ";
            }
        }
        bool simulate(const string& input) override {
    // Start with epsilon closure of start state
    set<string> currentStates = epsilonClosure({startState});
    
    cout << "\033[1;32m Simulating NFA with input: \033[0m \033[38;5;220m'" << input << "'\033[0m" << endl;
    cout << "\033[1;32m Start state(s) after epsilon closure: {";
    printStateSet(currentStates);
    cout << "}\033[0m" << endl;
    
    // Process each symbol in the input
    for (size_t i = 0; i < input.length(); i++) {
        char symbol = input[i];
        
        // Check if symbol is in alphabet
        if (alphabets.find(symbol) == alphabets.end()) {
            cout << "\033[38;5;196m Symbol '" << symbol << "' not in alphabet!\033[0m" << endl;
            return false;
        }

        // Print transition flow
        cout << "\033[1;36mStep " << (i+1) << ": On symbol '" << symbol << "' from state(s): {";
        printStateSet(currentStates);
        cout << "}";

        // Get all possible next states
        set<string> nextStates;
        for (const string& state : currentStates) {
            auto transition = transitions.find({state, symbol});
            if (transition != transitions.end()) {
                for (const string& toState : transition->second) {
                    if (toState != "nt") {
                        nextStates.insert(toState);
                    }
                }
            }
        }

        // Apply epsilon closure to all next states
        set<string> afterEpsilon = epsilonClosure(nextStates);

        cout << "  \033[1;33m Next state(s) before epsilon: {";
        printStateSet(nextStates);
        cout << "}  after epsilon: {";
        printStateSet(afterEpsilon);
        cout << "}\033[0m" << endl;

        currentStates = afterEpsilon;

        // If no states reachable, reject
        if (currentStates.empty()) {
            cout << "\033[38;5;220m No transitions available - REJECTED!\033[0m" << endl;
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

    cout << "\033[1;32m Final state(s): {";
    printStateSet(currentStates);
    cout << "}\033[0m" << endl;
    cout << "\033[1;32m Result: \033[0m" << (accepted ? " \033[1;32mACCEPTED\033[0m " : "\033[1;31m REJECTED\033[0m") << endl;
    return accepted;
}
set<int> listAvailableNFA(){
    string command = "python db_operation.py listNFA";
    cout << "\033[38;2;255;105;180m loading available NFA...\033[0m" << endl;
    sleepFor(2000);
    clearScreen();
    set<int> idSet;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        cout << "\033[38;5;196m Failed to run command!\033[0m" << endl;
        return idSet;
    }
    char buffer[128];
    string result;
    while(fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    if(result.find("EMPTY") != string::npos){
        cout << "\033[38;5;220mNo NFA in Database.\033[0m" <<endl;
        return idSet;
    }
    try{
        size_t automataStart = result.find("\"automata\":");
        if (automataStart == string::npos) {
            cout << "\033[38;5;220m Error: Could not find automata array in response\033[0m" << endl;
            return idSet;
        }

        // Print clean header
        cout << "\n\033[1;36m          Available Finite Automata\033[0m" << endl;
        cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;
        cout << "\033[35m|\033[0m \033[1;31mID   \033[0m| \033[1;31mType   \033[0m| \033[1;31mName\033[0m" << string(50 - 30, ' ') << "\033[35m|\033[0m\n";
        cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;

        // Find array bounds
        size_t arrayStart = result.find('[', automataStart);
        size_t arrayEnd = result.find(']', arrayStart);
        
        if (arrayStart == string::npos || arrayEnd == string::npos) {
            cout << " Error: Invalid JSON format" << endl;
            return idSet;
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
            idSet.insert(stoi(idStr));
            // Print clean formatted row
            cout << "\033[35m|\033[0m" // left border in magenta
                    << "\033[32m" << setw(6) << left << idStr << "\033[0m"
                    << "\033[35m|\033[0m"
                    << "\033[33m" << setw(8) << left << type << "\033[0m"
                    << "\033[35m|\033[0m"
                    << "\033[34m" << setw(25) << left << name << "\033[0m"
                    << "\033[35m|\033[0m"
                    << endl;

            pos = endPos + 1;
        }
        cout <<"\033[35m"<< string(43, '=')<<"\033[0m" << endl;
        return idSet;
    }catch(const exception& e) {
        cout << "\033[38;5;220m Error parsing NFA list: \033[0m" << endl;
    }
    return idSet;
}
// Helper function to compute epsilon closure
set<string> epsilonClosure(const set<string>& states) {
    set<string> closure = states; // Start with input states
    bool changed = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 100;
    
    while (changed && iterations < MAX_ITERATIONS) {
        changed = false;
        set<string> newStates = closure;
        iterations++;
        
        for (const string& state : closure) {
            if (isAllowEpsilonTransitions) {
                
                // Check for epsilon transitions
                auto epsilonTrans = transitions.find({state, 'e'});
                if (epsilonTrans != transitions.end()) {
                    for (const string& epsilonState : epsilonTrans->second) {
                        if (epsilonState != "nt" && newStates.find(epsilonState) == newStates.end()) {
                            newStates.insert(epsilonState);
                            changed = true;
                        }
                    }
                    cout << endl;
                }
            }
        }
        
        closure = newStates;
    }
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
            cout << "\n\033[1;36mTransition Table\033[0m" << endl;
            cout << "\033[1;35m-------------------\033[0m" << endl;
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
                            cout <<"\033[1;32m"<< fromState + "*"<<"\033[0m" <<"\033[1;33m"<< " --" << symbol << "--> " <<"\033[0m"<<"\033[1;31m"<< toState << "\033[0m"<<endl;
                        }else{
                            cout <<"\033[1;32m"<< fromState <<"\033[0m" <<"\033[1;33m"<< " --" << symbol << "--> " <<"\033[0m"<<"\033[1;31m"<< toState << "\033[0m"<<endl;
                        }
                    }
                }
            }
        }
        void handleInputForNFA(){
            cout << "\033[1;36m======== Designing NFA =========\033[0m"<<endl;
            bool isValid = false;
            do{
                cout << "\033[38;5;202mIs this NFA have epsilon transitions?(y/n): \033[0m";
                char choice;
                cin >> choice;
                if(choice == 'y' || choice == 'n'){
                    isValid = true;
                    isAllowEpsilonTransitions = choice == 'y' ? true : false;
                }else{
                    cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m"<<endl;
                }
            }while(!isValid);
            int numStates;
            do{
                numStates = getValidatedInt("\033[38;5;202mEnter number of states : \033[0m");
                if(numStates <= 0){
                    cout << "\033[38;5;226mError: Number of states must be greater than 0.\033[0m"<< endl;
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
                numAlphabet = getValidatedInt("\033[38;5;202mEnter number of symbols : \033[0m");
                if(numAlphabet <= 0){
                    cout << "\033[38;5;226mError: Number of symbols must be greater than 0.\033[0m" << endl;
                }
            }while(numAlphabet <=0 );
            numOfAlphabet = numAlphabet;
            for(int i = 0 ; i < numOfAlphabet ; i++){
                char symbol;
                cout << "\033[38;5;202mEnter symbol " << i+1 << ": \033[0m";
                cin >> symbol;
                addSymbol(symbol);
            }
            sleepFor(800);
            clearScreen();
            cout << "\033[1;32mYou have " << numOfStates << " states there're ";
            displayState();
            cout << "\033[1;32m and " << numOfAlphabet << " symbols there're \033[0m";
            displaySymbol();
            cout << "\033[1;32m in your NFA.\033[0m" << endl;
            string state_state;
            do{
                cout << "\033[38;5;202mEnter start state : \033[0m";
                cin >> state_state;
                if(states.find(state_state)== states.end()){
                    cout << "\033[38;5;226mError: Start state must be one of the defined states.\033[0m" << endl;
                }
            }while(states.find(state_state) == states.end());
            startState = state_state;
            int numOfAcc;
            do{
                numOfAcc = getValidatedInt("\033[38;5;202mEnter number of accepting states : \033[0m");
                if(numOfAcc <= 0){
                    cout << "\033[38;5;226mError: Number of accepting states must be greater than 0.\033[0m"<< endl;
                }
            }while(numOfAcc <= 0);
            numOfAcceptingStates = numOfAcc;
            for(int i = 0 ;i < numOfAcceptingStates ; i++){
                string acceptingState;
                do{
                    cout << "\033[38;5;202mEnter accepting state " << i+1 << " : \033[0m";
                    cin >> acceptingState;
                    if(states.find(acceptingState) == states.end()){
                        cout << "\033[38;5;226mError: Accepting state must be one of the defined states.\033[0m" << endl;
                    }
                }while(states.find(acceptingState) == states.end());
                addAcceptingStates(acceptingState);
            }
            bool isAny = false;
            for(const auto& state : states){
                if(state =="nt"){
                    continue;
                }
                for(const auto& alphabet : alphabets){
                    char addmore;
                    do{
                        string toState;
                        do{
                            cout << "\033[38;5;202mEnter transition from state "<<state << " with symbol "<<alphabet << " to state (if no transition enter \"nt\"): \033[0m";
                            cin >> toState;
                            if(states.find(toState)==states.end()){
                                cout << "\033[38;5;226mError: Transition state must be one of the defined states.\033[0m"<<endl;
                            }
                        }while(states.find(toState) == states.end());
                        addTransition(state, alphabet, toState);
                        do{
                            cout << "\033[38;5;202mis there any more transition from state " << state << " with symbol " << alphabet << "? (y/n): \033[0m";
                            cin >> addmore;
                            if(addmore != 'y' && addmore != 'n'){
                                cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                            }else{
                                isAny = true;
                            }  
                        }while(!isAny);
                    }while(addmore == 'y' || addmore == 'Y');
                }
            }
            sleepFor(1000);
            clearScreen();
            if(isAllowEpsilonTransitions) {
                addSymbol('e');
                cout << "\n\033[1;36m--- Adding Epsilon Transitions (ep) ---\033[0m" << endl;
                for(const auto& state : states){
                    if(state == "nt") continue;
                    
                    char hasEpsilon;
                    cout << "\033[38;5;202mDoes state " << state << " have epsilon transitions? (y/n): \033[0m";
                    cin >> hasEpsilon;
                    
                    if(hasEpsilon == 'y' || hasEpsilon == 'Y') {
                        char addMore = 'y';
                        while(addMore == 'y') {
                            string toState;
                            do{
                                cout << "\033[38;5;202mEnter epsilon transition from " << state << " to state: \033[0m";
                                cin >> toState;
                                if(states.find(toState) == states.end()){
                                    cout << "\033[38;5;226mError: Transition state must be one of the defined states.\033[0m" << endl;
                                    cout << "\033[1;32m";
                                    displayState();
                                    cout << "\033[0m" << endl;
                                }
                            }while(states.find(toState) == states.end());
                            
                            addTransition(state, 'e', toState);
                            cout << "\033[1;32m Added epsilon transition: \033[0m"<<"\033[38;5;226m" << state << " --epsilon--> " << toState << "\033[0m" << endl;
                            cout << "\033[38;5;202mAdd another epsilon transition from " << state << "? (y/n): \033[0m";
                            cin >> addMore;
                        }
                    }
                }
            }
            cout << "\n \033[1;32mNFA created successfully!\033[0m" << endl;
            displayTransitions();
            pauseAndClear();
            char testChoice;
            bool isacc;
            do{
                cout << "\n\033[38;5;202mDo you want to test this NFA? (y/n): \033[0m";
                cin >> testChoice;
                if(testChoice !='y' && testChoice != 'n'){
                    cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                    isacc = true;
                }else{
                    isacc = false;
                }
            }while(isacc);
            if(testChoice == 'y'){
                string testInput;
                do {
                    cout << "\033[38;5;202mEnter a string to test (or 'quit' to stop): \033[0m";
                    cin >> testInput;
                    if(testInput != "quit") {
                        cout <<"\033[1;36m"<< "\n" << string(30, '-')<<"\033[0m" << endl;
                        simulate(testInput);
                        cout <<"\033[1;36m"<< string(30, '-') << "\033[0m" << endl;
                    }
                } while(testInput != "quit");
            }
            pauseAndClear();
            char saveChoice;
            do{
                cout << "\n\033[38;5;202mDo you want to save this NFA to database? (y/n): \033[0m";
                cin >> saveChoice;
                if(saveChoice !='y' && saveChoice !='n'){
                    cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                    isacc = true;
                }else{
                    isacc = false;
                }
            }while(isacc);
            if(saveChoice == 'y' || saveChoice == 'Y') {
                saveToDatabase();
            }
        }
};

void menu() {
    clearScreen();
    cout << "\n\033[1;35m========================================\033[0m\n";
    cout << "\033[1;35m|                                      |\033[0m\n";
    cout << "\033[1;35m|   \033[1;36mFINITE AUTOMATON SIMULATOR         \033[1;35m|\033[0m\n";
    cout << "\033[1;35m|                                      |\033[0m\n";
    cout << "\033[1;35m========================================\033[0m\n";
    cout << "\033[1;35m| \033[31m1.\033[0m \033[34mDesign a FA                       \033[1;35m|\033[0m\n";
    cout << "\033[1;35m| \033[31m2.\033[0m \033[34mSimulate a FA from database       \033[1;35m|\033[0m\n";
    cout << "\033[1;35m| \033[31m3.\033[0m \033[34mConvert NFA to DFA                \033[1;35m|\033[0m\n";
    cout << "\033[1;35m| \033[31m4.\033[0m \033[34mCheck type of FA                  \033[1;35m|\033[0m\n";
    cout << "\033[1;35m| \033[31m5.\033[0m \033[34mMinimize DFA                      \033[1;35m|\033[0m\n";
    cout << "\033[1;35m| \033[91m0.\033[0m \033[94mExit                              \033[1;35m|\033[0m\n";
    cout << "\033[1;35m========================================\033[0m\n";
}

void designMenu() {
    clearScreen();
    cout << "\n\033[1;35m" << string(40, '=') <<"\033[0m"<< endl;
    cout << "\033[1;35m|                                      |\033[0m" << endl;
    cout << "\033[1;35m|\033[0m";
    cout << "\033[1;36m        Design Finite Automaton\033[0m       \033[1;35m|\033[0m" << endl;
    cout << "\033[1;35m|                                      |\033[0m" << endl;
    cout << "\033[1;35m" << string(40, '=') <<"\033[0m"<< endl;
    cout << "\033[1;35m|";
    cout << "\033[31m 1.\033[0m \033[34mCreate DFA\033[0m                        \033[1;35m|\033[0m" << endl;
    cout << "\033[1;35m|";
    cout << "\033[31m 2.\033[0m \033[34mCreate NFA\033[0m                        \033[1;35m|\033[0m" << endl;
    cout << "\033[1;35m|";
    cout << "\033[91m 0.\033[0m \033[94mBack to main menu\033[0m                 \033[1;35m|\033[0m" << endl;
    cout << "\033[1;35m" << string(40, '=') <<"\033[0m"<< endl;
}

void checkTypeMenu(){
    cout << "\033[1;36m===== Check FA type from Database ======\033[0m"<<endl;
    DFA temp;
    map<int,string> faTypes = temp.listAvailableFA();
    if(faTypes.empty()){
        cout << "\033[38;5;220m No Finite Automata available in the database!\033[0m" << endl;
        return;
    }
    int id;
    do{
        id = getValidatedInt("\033[38;5;202mEnter FA ID to load: \033[0m");
        if(faTypes.find(id)==faTypes.end()){
            cout << "\033[38;5;220m Invalid FA ID! Please choose from the available IDs.\033[0m" << endl;
        }
    }while(faTypes.find(id)==faTypes.end());
    
    string fatype = faTypes[id];
    FiniteAutoMaton* fa = nullptr;

    // Load as NFA first (since NFA can handle both types)
    if (fatype == "NFA") {
        fa = new NFA();
    } else {
        fa = new DFA();
        sleepFor(800);
        clearScreen();
    }
    
    cout << "\033[1;32m Loading FA for type analysis...\033[0m" << endl;
    sleepFor(500);
    clearScreen();
    fa->loadFromDatabase(id);
    
    // Cast to NFA for detailed analysis
    map<pair<string,char>, set<string>> analysisTransitions;
    if (fatype == "NFA") {
        NFA* nfa = static_cast<NFA*>(fa);
        analysisTransitions = nfa->getNFATransitions();
    } else {
        DFA* dfa = static_cast<DFA*>(fa);
        auto& dfaTrans = dfa->getTransitions();
        for (const auto& t : dfaTrans) {
            analysisTransitions[t.first].insert(t.second);
        }
    }

    cout << "\n\033[1;36m Analyzing FA Structure...\033[0m" << endl;
    sleepFor(2000);
    cout << "\033[35m================================\033[0m" << endl;
    
    bool hasEpsilonTransitions = false;
    bool hasMultipleTransitions = false;
    bool hasMissingTransitions = false;
    
    // Step 1: Check for epsilon transitions
    cout << "\033[38;2;255;105;180m Step 1: Checking for epsilon transitions...\033[0m" << endl;
    sleepFor(1500);
    for(const auto& transition : analysisTransitions) {
        if(transition.first.second == 'e') {
            hasEpsilonTransitions = true;
            cout << "\033[1;34m    Found epsilon transition: \033[0m" 
                <<"\033[1;32m"<< transition.first.first<<"\033[0m\033[1;33m" << " --e--> " << "\033[0m";
            for(const string& to : transition.second) {
                cout <<"\033[1;31m"<< to << "\033[0m ";
            }
            cout << endl;
        }
    }
    
    if(hasEpsilonTransitions) {
        cout << "\033[1;32m RESULT: This is an NFA (has epsilon transitions)\033[0m" << endl;
        delete fa;
        pauseAndClear();
        return;
    }
    cout << "\033[1;34m    No epsilon transitions found\033[0m" << endl;
    
    // Step 2: Check for multiple transitions from same state with same symbol
    cout << "\n\033[38;2;255;105;180m Step 2: Checking for nondeterministic transitions...\033[0m" << endl;
    sleepFor(1500);
    for(const auto& transition : analysisTransitions) {
        if(transition.second.size() > 1) {
            hasMultipleTransitions = true;
            cout << "\033[1;34m    Found multiple transitions: \033[0m" 
                <<"\033[1;32m"<< transition.first.first<<"\033[0m\033[1;33m" << " --" << transition.first.second << "--> \033[0m{" <<"\033[1;31m";
            bool first = true;
            for(const string& to : transition.second) {
                if(!first) cout << ", ";
                cout << to;
                first = false;
            }
            cout << "}\033[0m \033[38;5;220m (" << transition.second.size() << " destinations)\033[0m" << endl;
        }
    }
    
    if(hasMultipleTransitions) {
        cout << "\033[1;32m RESULT: This is an NFA (has nondeterministic transitions)\033[0m" << endl;
        delete fa;
        pauseAndClear();
        return;
    }
    cout << "\033[1;34m    No multiple transitions found\033[0m" << endl;
    
    // Step 3: Check for missing transitions (incomplete DFA)
    cout << "\n\033[38;2;255;105;180m Step 3: Checking for missing transitions...\033[0m" << endl;
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

            auto transition = analysisTransitions.find({state, symbol});
            if(transition != analysisTransitions.end() && !transition->second.empty()) {
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
                    cout << "\033[1;34m     Missing transition: δ(" << state << ", " << symbol << ") = undefined\033[0m" << endl;
                }
            } else {
                hasMissingTransitions = true;
                cout << "\033[1;34m     Missing transition: δ(" << state << ", " << symbol << ") = undefined\033[0m" << endl;
            }
        }
    }
    
    if(hasMissingTransitions) {
        cout << "\033[1;32m RESULT: This is an NFA (incomplete - has missing transitions)\033[0m" << endl;
        cout << "\033[1;32m   Expected transitions: \033[0m\033[1;31m" << expectedTransitions <<"\033[0m"<< endl;
        cout << "\033[1;32m   Actual transitions: \033[0m\033[1;31m" << actualTransitions << "\033[0m" << endl;
        delete fa;
        pauseAndClear();
        return;
    }
    cout << "\033[1;32m    All transitions are defined\033[0m" << endl;

    // Step 4: Final determination
    cout << "\n\033[38;2;255;105;180m Step 4: Final analysis...\033[0m" << endl;
    sleepFor(1500);
    cout << "\033[1;32m    No epsilon transitions\033[0m" << endl;
    cout << "\033[1;32m    No multiple transitions per state-symbol pair\033[0m" << endl;
    cout << "\033[1;32m    All transitions are defined\033[0m" << endl;
    cout << "\033[1;32m    Exactly one transition per state-symbol pair\033[0m" << endl;

    cout << "\n\033[1;32m RESULT: This is a DFA (Deterministic Finite Automaton)\033[0m" << endl;

    // Display comprehensive analysis
    cout << "\n\033[1;36m Detailed Analysis:\033[0m" << endl;
    cout << "\033[1;35m================================\033[0m" << endl;
    cout << "\033[1;32mType: DFA\033[0m" << endl;
    cout << "\033[1;32mStates: \033[0m\033[1;31m" << states.size() << "\033[0m" << endl;
    cout << "\033[1;32mAlphabet symbols: \033[0m\033[1;31m" << alphabet.size() << "\033[0m" << endl;
    cout << "\033[1;32mTotal transitions: \033[0m\033[1;31m" << actualTransitions << "\033[0m" << endl;
    cout << "\033[1;32mCompleteness: \033[0m\033[1;31mComplete\033[0m" << endl;
    cout << "\033[1;32mDeterminism: \033[0m\033[1;31mDeterministic\033[0m" << endl;
    pauseAndClear();
    delete fa;
}
void simulateMenu() {
    cout << "\n\033[1;36m===== Simulate a FA from Database ===\033[0m" << endl;
    DFA tempDFA;
    map<int,string> faTypes = tempDFA.listAvailableFA();

    if(faTypes.empty()){
        cout << "\033[1;33m No Finite Automata available in the database!\033[0m" << endl;
        return;
    }
    int id;
    do{
        id = getValidatedInt("\033[38;5;202m\nEnter FA ID to load: \033[0m");
        if (faTypes.find(id) == faTypes.end()) {
                cout << "\033[1;31m Invalid FA ID! Please choose from the available IDs.\033[0m" << endl;
        }
    }while(faTypes.find(id) == faTypes.end());
    string faType = faTypes[id];
    FiniteAutoMaton* fa = nullptr;

    if (faType == "NFA") {
        fa = new NFA();
    } else {
        fa = new DFA();
    }
    clearScreen();
    fa->loadFromDatabase(id);

    string testInput;
    do {
        cout << "\033[38;5;202mEnter a string to test (or 'quit' to stop): \033[0m";
        cin >> testInput;
        if(testInput != "quit") {
            if(fa->simulate(testInput)) {
                cout << " \033[92mACCEPTED!\033[0m" << endl;
            } else {
                cout << " \033[91mREJECTED!\033[0m" << endl;
            }
        }
    } while(testInput != "quit");
    delete fa;
    pauseAndClear();
}

void minimizeMenu(){
    cout << "\033[1;36m======= Minimize DFA ========\033[0m" << endl;
    DFA tempDFA;
    set<int> dfaIds = tempDFA.listAvailableDFA();
    int dfaId;
    do{
        dfaId = getValidatedInt("\033[38;5;202mEnter DFA's ID to Minimize: \033[0m");
        if(dfaIds.find(dfaId)== dfaIds.end()){
            cout << "\033[38;5;220m Invalid DFA ID! Please choose from the available IDs.\033[0m" << endl;
        }
    }while(dfaIds.find(dfaId) == dfaIds.end());
    clearScreen();
    tempDFA.loadFromDatabase(dfaId);
    string jsonData = tempDFA.toJSON("minimized_dfa.json");
    string tempFile = "dfa_input.json"; 
    ofstream jsonFile(tempFile);
    if(!jsonFile.is_open()){
        cout << "\033[38;5;220m Failed to create temporary JSON file!\033[0m" << endl;
        return;
    }
    jsonFile << jsonData;
    jsonFile.close();
    string command = "python dfa_minimizer.py";
    cout << "\033[38;2;255;105;180m Minimizing DFA... \033[0m" << endl;
    int result = system(command.c_str());
    if(result == 0){
        DFA minimizedDFA;
        ifstream input("minimized.json");
        if(!input){
            cerr << "\033[38;5;220merror : cannot open minimized_dfa.json\033[0m";
        }
        pauseAndClear();
        json j;
        input >> j;
        minimizedDFA.extractFromJson(j);
        cout << "\033[1;36mMinimized DFA details:\033[0m" << endl;
        minimizedDFA.getDetails();
        bool isValid;
        char displayChoice;
        do{
            cout << "\033[38;5;202mwould you like to see the minimized DFA? (y/n): \033[0m";
            cin >> displayChoice;
            if(displayChoice !='y' && displayChoice != 'n'){
                cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                isValid = true;
            }
            else{
                isValid = false;
            }
        }while(isValid);
        if(displayChoice == 'y'){
            string content = minimizedDFA.toJSON("minimized_dfa");
            string commandCall = "python display.py";
            string minimized = "dfa.json";
            ofstream jsonFile(minimized);
            if(!jsonFile.is_open()){
                cout << "\033[38;5;220mFailed to create temporary JSON file!\033[0m" << endl;
                return;
            }
            jsonFile << content;
            jsonFile.close();
            system(commandCall.c_str()); 
            remove(minimized.c_str());
        }
        remove(tempFile.c_str());
        remove("minimized.json");
        char saveChoice;
        do{
            cout << "\033[38;5;202mDo you want to save this minimized DFA to database? (y/n): \033[0m";
            cin >> saveChoice;
            if(saveChoice != 'y' && saveChoice != 'n'){
                cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                isValid = true;
            }else{
                isValid = false;
            }
        }while(isValid);
        if(saveChoice == 'y') {
            minimizedDFA.saveToDatabase();
        }else{
            pauseAndClear();
        }
    } else {
        cout << "\033[38;5;220mFailed to minimize DFA!\033[0m" << endl;
    }
}

void handleUserInputForMenu() {
    int choice;
    do {
        menu();
        choice = getValidatedInt("\033[38;5;202mPlease enter your choice: \033[0m");
        switch(choice) {
            case 1: {
                int designChoice;
                do {
                    designMenu();
                    designChoice = getValidatedInt("\033[38;5;202mPlease enter your choice: \033[0m");
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
                            cout << "\033[1;32mReturning to main menu...\033[0m" << endl;
                            sleepFor(1000);
                            clearScreen();
                            break;
                        default:
                            cout << "\033[38;5;226mInvalid choice! Please try again.\033[0m" << endl;
                            pauseAndClear();
                            break;
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
                cout << "\033[1;36m========= Convert NFA to DFA =========\033[0m" << endl;
                NFA nfa;
                int nfaId;
                set<int> nfaIds = nfa.listAvailableNFA();
                do{
                    nfaId = getValidatedInt("\033[38;5;202mEnter NFA's ID to convert: \033[0m");
                    if(nfaIds.find(nfaId) == nfaIds.end()){
                        cout << "\033[38;5;220m Invalid NFA ID! Please choose from the available IDs.\033[0m" << endl;
                    }
                } while(nfaIds.find(nfaId) == nfaIds.end());

                nfa.loadFromDatabase(nfaId);
                string jsonData = nfa.toJSON("converted_dfa.json");
                string tempFile = "nfa_input.json";
                ofstream jsonFile(tempFile);
                if(!jsonFile.is_open()){
                    cout << "\033[38;5;220m Failed to create temporary JSON file!\033[0m" << endl;
                    return;
                }
                jsonFile << jsonData;
                jsonFile.close();
                string command = "python nfa_to_dfa.py";
                cout << "\033[38;2;255;105;180m Converting NFA to DFA...\033[0m" << endl;
                int result = system(command.c_str());
                if(result == 0) {
                    DFA dfa;
                    ifstream input("dfa_output.json");
                    if(!input){
                        cerr << "error : cannot open dfa.json";
                    }
                    pauseAndClear();
                    json j;
                    input >> j;
                    dfa.extractFromJson(j);
                    cout << "\033[1;36mConverted DFA details:\033[0m" << endl;
                    dfa.getDetails();
                    char displayChoice;
                    bool isValid;
                    do{
                        cout << "\033[38;5;202mWould you like to see the converted DFA? (y/n): \033[0m";
                        cin >> displayChoice;
                        if(displayChoice !='y' && displayChoice != 'n'){
                            cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                            isValid = true;
                        }else{
                            isValid = false;
                        }
                    }while(isValid);
                    if(displayChoice == 'y') {
                        string content = dfa.toJSON("convereted dfa");
                        string commandCall = "python display.py";
                        string dfaFile = "dfa.json";
                        ofstream dfaJsonFile(dfaFile);
                        if(!dfaJsonFile.is_open()){
                            cout << " Failed to create temporary JSON file!" << endl;
                            return;
                        }
                        dfaJsonFile << content;
                        dfaJsonFile.close();
                        system(commandCall.c_str());
                        remove(dfaFile.c_str());
                    }
                    remove(tempFile.c_str());
                    remove("dfa_output.json");
                    pauseAndClear();
                    char saveChoice;
                    do{
                        cout << "\033[38;5;202mDo you want to save this DFA to database? (y/n): \033[0m";
                        cin >> saveChoice;
                        if(saveChoice != 'y' && saveChoice != 'n'){
                            cout << "\033[38;5;226mError: Invalid choice. Please enter y or n.\033[0m" << endl;
                            isValid = true;
                        }else{
                            isValid = false;
                        }
                    }while(isValid);
                    if(saveChoice == 'y' || saveChoice == 'Y') {
                        dfa.saveToDatabase();
                        cout << "\033[38;5;220m DFA saved to database successfully!\033[0m" << endl;
                        pauseAndClear();
                    }else if(saveChoice == 'n' || saveChoice == 'N') {
                        pauseAndClear();
                    }
                } else {
                    cout << "\033[38;5;220m Failed to convert NFA to DFA!\033[0m" << endl;
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
                sleepFor(500);
                clearScreen();
                minimizeMenu();
                break;
            }
            case 0: {
                cout << " Thank you for using FA Simulator! Goodbye!" << endl;
                break;
            }
            default: {
                cout << "\033[38;5;220mInvalid choice! Please enter a number between 0-5.\033[0m" << endl;
                pauseAndClear();
                break;
            }
        }
    } while(choice != 0);
}

// Main function
int main() {
    handleUserInputForMenu();
    return 0;
}