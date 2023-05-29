#define INTERVAL_DURATION 10;
#define HIGHLIGHT_STRENGTH_FACTOR 2;

#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <Windows.h>
#include <conio.h>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

using namespace std;

string secondsToTime(int totalSeconds) {
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    stringstream ss;
    ss << setfill('0') << setw(2) << hours << ":"
        << setfill('0') << setw(2) << minutes << ":"
        << setfill('0') << setw(2) << seconds;

    return ss.str();
}

inline bool exists_test0(const std::string& name) {
    ifstream f(name.c_str());
    return f.good();
}


struct YourHashFunction {
    size_t operator()(const string& str) const {
        size_t hash = 0;
        for (const char c : str) {
            hash = (hash * 131) + c; // Modify the prime number as needed
        }
        return hash;
    }
};

std::chrono::high_resolution_clock::time_point startTime;

void startTimer() {
    startTime = std::chrono::high_resolution_clock::now();
}

long long stopTimer() {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    return duration;
}


int main()
{
    long long hours, minutes, seconds;
    vector<double> intervals;
    double messageCount{ 0 };
    double totalSeconds{ 0 };
    double intervalIndex{ 0 };
    double numIntervals{ 0 };
    double intervalStart{ 0 };
    double intervalEnd{ 0 };
    double currentInterval{ 0 };
    vector<int> messageCounts;
    vector<double> secondsList; // list of seconds in each message
    vector<string> messages;
    string line, fileName;
    stringstream fileDirectory{};
    ifstream inFile;
    int dec, intervalsShown{ 0 };
    string searchMessage{ "" };
    string message{ "" };
    vector<string> mostUsedPhrase;
    char func;
    double compareIntervalDuration = 3600;

    double HSF = HIGHLIGHT_STRENGTH_FACTOR;
    double intervalDuration = INTERVAL_DURATION;

    cout << endl << "s - search phrase, d - default values from settings.txt, h - defaults with a compareInterval value of 1 hour , c - custom values:" << endl <<endl;

    while (true)
    {
        secondsList.clear();

        cout << "Enter VOD ID , highlight strength factor, and interval duration: ";
        cin >> fileName >> func;
        
        if (func == 'c')
        {
            cin >> HSF >> intervalDuration;
        }
        else if (func == 'd' || func == 'h')
        {
			fstream sFile("settings.txt", ios::in);
			sFile >> HSF >> intervalDuration;
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
            messages.push_back(std::move(message));
            secondsList.push_back(totalSeconds);

        }

        inFile.close();

        currentInterval = 0;
        int currentCompareInterval = 0;
        int x = 10; // Number of top phrases to save
        int numMessages = secondsList.size();
        double averageFrequency = numMessages / (double)secondsList.back();
        numIntervals = (numMessages*3 + intervalDuration - 1) / intervalDuration;
        int errorIndex = -1;
        vector<string> mostUsedPhrase(numIntervals);
        vector<int> messageFrequency(numIntervals, 0);
        vector<int> phraseCount(numIntervals);
        vector<unordered_map<string, int, YourHashFunction>> intervalPhraseCounts(numIntervals);
        unordered_map<string, int> phraseCountInterval; // Map to store phrase count within the interval
        vector<vector<string>> topPhrases(numIntervals); // Vector to store top X phrases for each interval
        vector<int> compareInterval;
        compareInterval.resize((totalSeconds/ compareIntervalDuration) * 10);

        cout << "Average message frequency: " << averageFrequency * intervalDuration << " messages per interval" << endl;

        if (func == 'c' || func == 'd' || func == 'h')
        {
            try {

                for (int i = 0; i < numMessages; i++)
                {
                    if (secondsList[i] >= (currentInterval + 1) * intervalDuration)
                    {
                        currentInterval++;
                        if (currentInterval >= messageFrequency.size() || currentInterval >= intervalPhraseCounts.size() || currentInterval >= phraseCount.size() || currentInterval >= mostUsedPhrase.size() || currentInterval >= topPhrases.size())
                        {
                            // Handle vector size mismatch or out of range access
                            errorIndex = i;
                            throw std::out_of_range("Vector out of range error: Current Interval exceeds vector sizes");
                        }
                    }
                    if (secondsList[i] >= (currentCompareInterval + 1) * compareIntervalDuration)
                    {
                        currentCompareInterval++;
                    }
                    messageFrequency[currentInterval]++;
                    compareInterval[currentCompareInterval]++;

                    string currentPhrase = messages[i];

                    auto& phraseCountMap = intervalPhraseCounts[currentInterval];
                    istringstream iss(currentPhrase);
                    string word;
                    while (iss >> word)
                    {
                        phraseCountMap[word]++; // Increment the count of each individual word/phrase
                    }

                    int currentPhraseCount = phraseCountMap[currentPhrase]; // Get the count of the current phrase
                    if (currentInterval >= phraseCount.size() || currentInterval >= mostUsedPhrase.size())
                    {
                        // Handle vector size mismatch or out of range access
                        errorIndex = i;
                        throw std::out_of_range("Vector out of range error: Current Interval exceeds vector sizes");
                    }
                    int& mostUsedPhraseCount = phraseCount[currentInterval]; // Get the count of the current most used phrase

                    if (currentPhraseCount > mostUsedPhraseCount)
                    {
                        mostUsedPhraseCount = currentPhraseCount; // Update the count of the most used phrase
                        mostUsedPhrase[currentInterval] = currentPhrase; // Update the most used phrase
                    }

                    // Update top phrases for the current interval
                    if (currentInterval >= topPhrases.size())
                    {
                        // Handle vector size mismatch or out of range access
                        errorIndex = i;
                        throw std::out_of_range("Vector out of range error: Current Interval exceeds vector sizes");
                    }
                    auto& topPhrasesInterval = topPhrases[currentInterval];
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

                    // Sort the top phrases in descending order of counts
                    sort(topPhrasesInterval.begin(), topPhrasesInterval.end(), [&](const string& a, const string& b) {
                        return phraseCountInterval[a] > phraseCountInterval[b];
                        });

                    // Keep only the top X phrases
                    if (topPhrasesInterval.size() > x)
                    {
                        topPhrasesInterval.resize(x);
                    }
                }
            }
            catch (const std::out_of_range& e) {
                std::cout << "Error occurred at index " << errorIndex << ": " << e.what() << ": " << messages[errorIndex] << std::endl;
            }
            catch (const std::exception& e) {
                std::cout << "Exception occurred: " << e.what() << std::endl;
            }

            messageFrequency.reserve(numIntervals);
            mostUsedPhrase.reserve(numIntervals);
            phraseCount.reserve(numIntervals);
            intervalPhraseCounts.reserve(numIntervals);

            inFile.close();

            int previousHour = -1; // Initialize with an invalid hour
            double compareValue = 0;

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

                if (func == 'h')
                {
                    compareValue = ((compareInterval[currentHour] / compareIntervalDuration) * intervalDuration) * HSF;
                }
                else
                {
                    compareValue = (averageFrequency * intervalDuration)* HSF;
                }

                if (messageFrequency[i] > compareValue)
                {
                    cout << endl << "Interval [" << secondsToTime(intervalStart) << " - " << secondsToTime(intervalEnd) << "]: " << messageFrequency[i] << " messages ";
               
                    const auto& topPhrasesInterval = topPhrases[i];
                    for (int j = 0; j < min(x, static_cast<int>(topPhrasesInterval.size())); j++)
                    {
                        const string& phrase = topPhrasesInterval[j];
                        int phraseCount = intervalPhraseCounts[i][phrase];
                        cout << phrase << "[" << phraseCount << "] ";
                    }
                }
            }
            for (size_t i = 0; i < currentCompareInterval; i++)
            {
                cout << endl << "Interval [" << secondsToTime(i * compareIntervalDuration) << " - " << secondsToTime((i * compareIntervalDuration) + compareIntervalDuration) << "]: " << compareInterval[i] << " messages ";
            }


            cout << endl << endl;
		}

        else if (func == 's')
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
                    if (i < messages.size())
                    {
                        string message = messages.at(i);

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

                        cout << "Interval [" << intervalStartTime << " - " << intervalEndTime << "]: ";

                        if (i < intervalMessages.size())
                        {
                            for (const auto& message : intervalMessages.at(i))
                            {
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

        compareInterval.clear();
        messages.clear();
        secondsList.clear();
        messageFrequency.clear();
        mostUsedPhrase.clear();
        phraseCount.clear();
        intervalPhraseCounts.clear();

    }
    return 0;
}