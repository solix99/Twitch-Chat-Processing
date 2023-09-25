#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <semaphore>
#include <thread>
#include <atomic>
#include <memory_resource>


using namespace std;

struct YourHashFunction
{
    size_t operator()(const string& str) const 
    {
        size_t hash = 0;
        for (const char c : str) 
        {
            hash = (hash * 131) + c; // Modify the prime number as needed
        }
        return hash;
    }
};

constexpr int INTERVAL_DURATION = 10;
constexpr int HIGHLIGHT_STRENGTH_FACTOR = 2;
constexpr int showPhraseCountAmmount = 10;

const int PROCESS_THREAD_COUNT = 4;

double HSF = HIGHLIGHT_STRENGTH_FACTOR;
atomic<int> intervalDuration = INTERVAL_DURATION;

long long hours, minutes, seconds;
vector<double> intervals;

std::atomic<int> messageCount{0};
std::atomic<int> totalSeconds{0};
std::atomic<int> intervalIndex{0};
std::atomic<int> numIntervals{0};
std::atomic<int> intervalStart{0};
std::atomic<int> intervalEnd{0};
std::atomic<int> currentInterval{0};

vector<int> messageCounts;
vector<double> secondsList; // list of seconds in each message
vector<string> userMessages;
string line, fileName, temp;
stringstream fileDirectory{};
ifstream inFile;
int dec, intervalsShown{ 0 };
string searchMessage{ "" };
string message{ "" };
vector<string> mostUsedPhrase;
vector<string> ignoreList;
char func[2];
atomic<int> compareIntervalDuration = 3600;
atomic<int> currentCompareInterval;

int numMessages;
double averageFrequency;
int errorIndex = -1;
vector<int> parts;
atomic<int> endIndex[PROCESS_THREAD_COUNT];
atomic<int> startIndex[PROCESS_THREAD_COUNT];
vector<int> messageFrequency;
vector<int> phraseCount;
vector<unordered_map<string, int, YourHashFunction>> intervalPhraseCounts;
unordered_map<string, int> phraseCountInterval;
vector<vector<string>> topPhrases;
atomic<int> compareInterval[50];
thread processThread[PROCESS_THREAD_COUNT];
counting_semaphore processDataSemaphore(PROCESS_THREAD_COUNT+1);


string secondsToTime(int totalSeconds) 
{
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    stringstream ss;
    ss << setfill('0') << setw(2) << hours << ":"
        << setfill('0') << setw(2) << minutes << ":"
        << setfill('0') << setw(2) << seconds;

    return ss.str();
}

inline bool exists_test0(const std::string& name) 
{
    ifstream f(name.c_str());
    return f.good();
}


bool checkStringInVector(const std::string& str, const std::vector<std::string>& vec)
{
    std::unordered_set<std::string> strSet(vec.begin(), vec.end());
    return strSet.find(str) != strSet.end();
}

std::chrono::high_resolution_clock::time_point startTime;

void startTimer() 
{
    startTime = std::chrono::high_resolution_clock::now();
}

void stopTimer(int threadIndex) 
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    cout << endl<<"Task took:"<< duration << " ms." << "["<<threadIndex<<"]";
}


void processData(int threadIndex)
{
    startTimer();

    //cout << endl << "Process started from:" << startIndex[threadIndex] << "-" << endIndex[threadIndex];

    processDataSemaphore.acquire();

    int threadIntervalDuration = intervalDuration;
    int threadMessageFrequencySize = messageFrequency.size();
    int threadStartIndex = startIndex[threadIndex];
    int threadEndIndex = endIndex[threadIndex];
    int threadCurrentInterval = secondsList[startIndex[threadIndex]] / threadIntervalDuration;
    int nextIntervalDuration = (threadCurrentInterval + 1) * threadIntervalDuration;

    processDataSemaphore.release();

    for (int i = threadStartIndex; i < threadEndIndex; i++)
    {
        if (secondsList[i] >= nextIntervalDuration)
        {
            threadCurrentInterval++;

           if (threadCurrentInterval >= threadMessageFrequencySize )
           {
               break;
           }

            nextIntervalDuration = (threadCurrentInterval + 1) * threadIntervalDuration;
        }

        messageFrequency[threadCurrentInterval]++;

        string currentPhrase = userMessages[i];

        auto& phraseCountMap = intervalPhraseCounts[threadCurrentInterval];
        istringstream iss(currentPhrase);
        string word;

        while (iss >> word)
        {
            if (!checkStringInVector(word, ignoreList))
            {
                phraseCountMap[word]++; // Increment the count of each individual word/phrase
            }
            else
            {
                messageFrequency[threadCurrentInterval]--;
            }
        }

        int currentPhraseCount = phraseCountMap[currentPhrase]; // Get the count of the current phrase

        int& mostUsedPhraseCount = phraseCount[threadCurrentInterval]; // Get the count of the current most used phrase

        // Update top phrases for the current interval
        if (threadCurrentInterval < topPhrases.size())
        {
            auto& topPhrasesInterval = topPhrases[threadCurrentInterval];
            unordered_map<string, int> phraseCountInterval; // Map to store phrase count within the interval

            for (const auto& phrase : topPhrasesInterval)
            {
                phraseCountInterval[phrase] = phraseCountMap[phrase]; // Store the count of each top phrase within the interval
            }

            if (phraseCountInterval.find(currentPhrase) == phraseCountInterval.end())
            {
                currentPhrase = currentPhrase.substr(0, 10);
                topPhrasesInterval.push_back(currentPhrase); // Add the current phrase to the top phrases if it's not already present
                phraseCountInterval[currentPhrase] = currentPhraseCount; // Store its count within the interval
            }

            sort(topPhrasesInterval.begin(), topPhrasesInterval.end(), [&](const string& a, const string& b) {
                return phraseCountInterval[a] > phraseCountInterval[b];
                });

            // Keep only the top X phrases
            if (topPhrasesInterval.size() > showPhraseCountAmmount)
            {
                topPhrasesInterval.resize(showPhraseCountAmmount);
            }
        }
    }
    stopTimer(threadIndex);

}

int main()
{
    cout << endl << intervalDuration;
    cout << endl << "s - search phrase, d - default values from settings.txt, h - defaults with a compareInterval value of 1 hour , c - custom values:" << endl <<endl;

    while (true)
    {
        secondsList.clear();

        cout << "Enter VOD ID , highlight strength factor, and interval duration: ";
        cin >> fileName;

        if (fileName == "addignore")
        {
            fstream ignoreFile("ignore.txt", ios::out | ios::app);
            string ignoreText[99];
            size_t k = 0;
            
            while (cin >> ignoreText[k])
            {
                if (ignoreText[k] == "end") { break; };

                ignoreFile << ignoreText[k] << ' ';

                k++;
            }

            ignoreFile.close();

            continue;
        }
        else
        {
            cin >> func;
        }

        fstream ignoreFile("ignore.txt", ios::in);
        while (ignoreFile >> temp)
        {
            ignoreList.push_back(temp);
        }
        ignoreFile.close();
        
        int tempIntervalDuration;

        if (func[0] == 'c')
        {
            cin >> HSF >> tempIntervalDuration >> func;
            intervalDuration.store(tempIntervalDuration, memory_order_relaxed);
        }
        else if (func[0] == 'd' || func[0] == 'h')
        {
			fstream sFile("settings.txt", ios::in);
			sFile >> HSF >> tempIntervalDuration;
            intervalDuration.store(tempIntervalDuration, memory_order_relaxed);
			sFile.close();
		}

        fileDirectory.str("");
        fileDirectory << fileName << ".txt";

        if (!exists_test0(fileDirectory.str()))
        {
            ofstream batch_file;
            batch_file.open("commands.cmd", ios::trunc);
            batch_file << "TwitchDownloaderCLI.exe chatdownload --id " << fileName << " -o " << fileName << ".txt" << endl;
            batch_file.close();

            cout << endl << batch_file._Stdstr;

            int batch_exit_code = system("cmd.exe /c commands.cmd"); // blocks until the child process is terminated
            if (batch_exit_code != 0) {
                cout << "Batch file exited with code " << batch_exit_code << endl;
            }

            remove("commands.cmd"); // delete the batch file
        }

        inFile.open(fileDirectory.str());
        getline(inFile, line);

        // Remove the first character from the input string
        line = line.substr(3);

        startTimer();

        while (getline(inFile, line))
        {
            int result = sscanf_s(line.c_str(), "[%lld:%lld:%lld]", &hours, &minutes, &seconds);

            // Extract username and message
            size_t pos1 = line.find(' ') + 1; // position of first space after timestamp
            size_t pos2 = line.find(':', pos1); // position of colon after username
            string username = line.substr(pos1, pos2 - pos1);
            string message = line.substr(pos2 + 2); // position of first character of message
            std::transform(message.begin(), message.end(), message.begin(), ::tolower);

            // Calculate total seconds and interval index
            totalSeconds = hours * 3600 + minutes * 60 + seconds;
            intervalIndex = totalSeconds / intervalDuration;

            // Update interval count
            if (intervalIndex >= intervals.size())
            {
                intervals.resize(intervalIndex + 1);
            }
            intervals[intervalIndex]++;
            messageCount++;

            // Store username, message, and total seconds
            userMessages.push_back(std::move(message));
            secondsList.push_back(totalSeconds);

        }
        stopTimer(-1);

        inFile.close();

        currentInterval = 0;
        numIntervals = (totalSeconds) / intervalDuration;
        currentCompareInterval = 0;
        numMessages = secondsList.size();
        averageFrequency = numMessages / (double)secondsList.back();
        mostUsedPhrase.resize(numIntervals);
        messageFrequency.resize(numIntervals,0);
        phraseCount.resize(numIntervals);
        intervalPhraseCounts.resize(numIntervals);
        topPhrases.resize(numIntervals);

        cout << "Average message frequency: " << averageFrequency * intervalDuration << " messages per interval" << endl;

        startIndex[0] = 0;

        for (int i = 0; i < PROCESS_THREAD_COUNT; ++i)
        {
            if (i < numMessages % PROCESS_THREAD_COUNT)
            {
                parts.push_back(numMessages / PROCESS_THREAD_COUNT + 1);
            }
            else
            {
                parts.push_back(numMessages / PROCESS_THREAD_COUNT);
            }
            startIndex[i + 1] = startIndex[i] + parts[i];
            endIndex[i] = startIndex[i] + parts[i];
        }

        if (func[0] == 'c' || func[0] == 'd' || func[0] == 'h')
        {
            try
            {
                for (size_t i = 0; i < PROCESS_THREAD_COUNT; i++)
                {
                    processThread[i] = thread(processData, i);
                }

                for (size_t i = 0; i < PROCESS_THREAD_COUNT; i++)
                {
                    processThread[i].join();
                }
            }
            catch (const std::out_of_range& e) {
                std::cout << "Error occurred at index " << errorIndex << ": " << e.what() << ": " << userMessages[errorIndex] << std::endl;
            }
            catch (const std::exception& e) {
                std::cout << "Exception occurred: " << e.what() << std::endl;
            }
            numIntervals = (totalSeconds) / intervalDuration;

            inFile.close();

            int previousHour = -1; // Initialize with an invalid hour

            double compareValueH = 0;
            double compareValueD = 0;


            for (int i = 0; i < numIntervals - 1; i++)
            {
                intervalStart = i * intervalDuration;
                intervalEnd = (i * intervalDuration) + intervalDuration;

                int currentHour = intervalStart / 3600;

                if (currentHour != previousHour)
                {
                    if (totalSeconds / 3600 > currentHour)
                    {
                        cout << endl;
                    }
                    previousHour = currentHour;
                }

                compareValueH = ((compareInterval[currentHour] / compareIntervalDuration) * intervalDuration) * HSF;
                compareValueD = (averageFrequency * intervalDuration) * HSF;

                try {
                    if (((messageFrequency[i] > compareValueH) && (func[1] == 'h' || func[0] == 'h')) || ((messageFrequency[i] > compareValueD) && func[0] == 'd'))
                    {
                        cout << endl << "Interval [" << secondsToTime(intervalStart) << " - " << secondsToTime(intervalEnd) << "]: " << messageFrequency[i] << " messages ";

                        const auto& topPhrasesInterval = topPhrases[i];
                        for (int j = 0; j < min(showPhraseCountAmmount, static_cast<int>(topPhrasesInterval.size())); j++)
                        {
                            const string& phrase = topPhrasesInterval[j];
                            int phraseCount = intervalPhraseCounts[i][phrase];
                            cout << phrase << "[" << phraseCount << "] ";
                        }
                    }
                }
                catch (const std::exception& e) {
                    cout << "An error occurred: " << e.what() << endl;
                }
                catch (...) {
                    cout << "An unknown error occurred." << endl;
                }
            }
            cout << endl << endl;
		}

        else if (func[0] == 's')
        {
            // Declare and initialize intervalMessages
            vector<vector<string>> intervalMessages(numIntervals);

            string searchPhrase; // Replace this with the search phrase you want to find

            cin >> searchPhrase;

            try {
                // Iterate over each message
                for (int i = 0; i < numMessages; i++)
                {
                    // Get the interval index for the current message
                    int intervalIndex = secondsList[i] / intervalDuration;

                    // Increment the message frequency count for the current interval
                    if (intervalIndex < messageFrequency.size())
                        messageFrequency.at(intervalIndex)++;

                    // Get the message text
                    if (i < userMessages.size())
                    {
                        string message = userMessages.at(i);

                        // Check if the search phrase is present in the message
                        if (message.find(searchPhrase) != string::npos)
                        {
                            // Increment the count for the search phrase in the current interval
                            if (intervalIndex < intervalPhraseCounts.size())
                                intervalPhraseCounts.at(intervalIndex)[searchPhrase]++;

                            // Store the message in the corresponding interval
                            if (intervalIndex < intervalMessages.size())
                                intervalMessages.at(intervalIndex).push_back(message);
                        }
                    }
                }

                // Iterate over each interval to find the most used phrase
                for (int i = 0; i < numIntervals; i++)
                {
                    int maxCount = 0;
                    string mostUsed;

                    // Check if the interval index is valid
                    if (i < intervalPhraseCounts.size() && i < mostUsedPhrase.size() && i < phraseCount.size())
                    {
                        // Iterate over the phrase counts within the interval
                        for (const auto& pair : intervalPhraseCounts.at(i))
                        {
                            // Check if the count is greater than the current max count
                            if (pair.second > maxCount)
                            {
                                maxCount = pair.second;
                                mostUsed = pair.first;
                            }
                        }

                        // Store the most used phrase for the current interval
                        mostUsedPhrase.at(i) = mostUsed;
                        phraseCount.at(i) = maxCount;
                    }
                }

                // Print out the intervals where the search phrase was most used and the corresponding messages
                for (int i = 0; i < numIntervals; i++)
                {
                    if (i < mostUsedPhrase.size() && mostUsedPhrase.at(i) == searchPhrase)
                    {
                        int intervalSeconds = i * intervalDuration;
                        string intervalStartTime = secondsToTime(intervalSeconds);
                        string intervalEndTime = secondsToTime(intervalSeconds + intervalDuration - 1);

                        if (i < intervalMessages.size())
                        {
                            for (const auto& message : intervalMessages.at(i))
                            {
                                cout << "Interval [" << intervalStartTime << " - " << intervalEndTime << "]: ";
                                cout << "   " << message << endl;
                            }
                        }
                    }
                }

            }
            catch (const std::out_of_range& e) {
                // Handle vector out of range error
                cout << "Error: Vector out of range." << endl;
            }

        }

        userMessages.clear();
        secondsList.clear();
        messageFrequency.clear();
        mostUsedPhrase.clear();
        phraseCount.clear();
        intervalPhraseCounts.clear();
        topPhrases.clear();
        phraseCountInterval.clear();
        intervalPhraseCounts.clear();

    }
    return 0;
}

