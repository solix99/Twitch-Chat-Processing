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

    double HSF = HIGHLIGHT_STRENGTH_FACTOR;
    double intervalDuration = INTERVAL_DURATION;

    while (true)
    {

        secondsList.clear();

        dec = 1;

        if (dec == 1)
        {
            cout << "Enter VOD ID , highlight strength factor, and interval duration: ";
            cin >> fileName >> HSF >> intervalDuration;
        }
        else if (dec == 2)
        {
            cout << "Enter VOD ID , interval duration, exact message, intervals shown: ";
            cin >> fileName >> intervalDuration >> searchMessage >> intervalsShown;
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
            intervalIndex = totalSeconds / INTERVAL_DURATION;

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


        // Create arrays that hold the frequency of messages in intervals
        int numMessages = secondsList.size();
        numIntervals = ceil(secondsList.back() / (double)intervalDuration);
        vector<int> messageFrequency(numIntervals, 0);
        currentInterval = 0;

        vector<string> mostUsedPhrase(numIntervals);
        vector<int> phraseCount(numIntervals);
        vector<unordered_map<string, int, YourHashFunction>> intervalPhraseCounts(numIntervals);

        for (int i = 0; i < numMessages; i++)
        {
            if (secondsList[i] >= (currentInterval + 1) * intervalDuration)
            {
                currentInterval++;
            }
            messageFrequency[currentInterval]++;

            string currentPhrase = messages[i];

            auto& phraseCountMap = intervalPhraseCounts[currentInterval];
            istringstream iss(currentPhrase);
            string word;
            while (iss >> word)
            {
                phraseCountMap[word]++; // Increment the count of each individual word/phrase
            }

            int currentPhraseCount = phraseCountMap[currentPhrase]; // Get the count of the current phrase
            int& mostUsedPhraseCount = phraseCount[currentInterval]; // Get the count of the current most used phrase

            if (currentPhraseCount > mostUsedPhraseCount)
            {
                mostUsedPhraseCount = currentPhraseCount; // Update the count of the most used phrase
                mostUsedPhrase[currentInterval] = currentPhrase; // Update the most used phrase
            }
        }

        messageFrequency.reserve(numIntervals);
        mostUsedPhrase.reserve(numIntervals);
        phraseCount.reserve(numIntervals);
        intervalPhraseCounts.reserve(numIntervals);

        inFile.close();

        if (dec == 1)
        {
            // Calculate average message frequency across all timestamps
            int numMessages = secondsList.size();
            double averageFrequency = numMessages / (double)secondsList.back();
            cout << "Average message frequency: " << averageFrequency * intervalDuration << " messages per interval" << endl;

            // Print the frequency of messages in each interval
            for (int i = 0; i < numIntervals; i++)
            {
                intervalStart = i * intervalDuration;
                intervalEnd = min((i + 1) * intervalDuration - 1, secondsList.back());
                //cout << "Interval [" << intervalStart << "s - " << intervalEnd << "s]: " << messageFrequency[i] << " messages" << endl;

                if (messageFrequency[i] > (averageFrequency * intervalDuration) * HSF)
                {
                    cout << endl << "Interval [" << secondsToTime(intervalStart) << " - " << secondsToTime(intervalEnd) << "]: " << messageFrequency[i] << " messages " << mostUsedPhrase[i].substr(0, 10) << "[" << phraseCount[i] << "]" << endl;
                }

            }
        }
        if (dec == 2)
        {
            int numMessages = secondsList.size();
            numIntervals = ceil(secondsList.back() / (double)intervalDuration);

            // Vector to store intervals and their corresponding message counts
            vector<pair<pair<int, int>, int>> intervalCounts;

            // Check each interval for the search message and keep count
            for (int i = 0; i < numIntervals; i++)
            {
                int intervalStart = i * intervalDuration;
                int intervalEnd = min((i + 1) * intervalDuration - 1, secondsList.back());
                int messageCount = 0;

                for (int j = 0; j < numMessages; j++)
                {
                    if (secondsList[j] >= intervalStart && secondsList[j] <= intervalEnd)
                    {
                        std::string lowerMessage = messages[j];
                        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);

                        std::string lowerSearchMessage = searchMessage;
                        std::transform(lowerSearchMessage.begin(), lowerSearchMessage.end(), lowerSearchMessage.begin(), ::tolower);

                        size_t pos = lowerMessage.find(lowerSearchMessage, 0);
                        while (pos != std::string::npos)
                        {
                            messageCount++;
                            pos = lowerMessage.find(lowerSearchMessage, pos + 1);
                        }
                    }
                }

                intervalCounts.push_back(make_pair(make_pair(intervalStart, intervalEnd), messageCount));
            }

            // Sort the intervalCounts vector in descending order based on message count
            sort(intervalCounts.begin(), intervalCounts.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.second > rhs.second;
                });

            // Print the desired number of intervals from the sorted vector
            int intervalsPrinted = 0;
            for (const auto& intervalCount : intervalCounts)
            {
                if (intervalCount.second > 0 && intervalsPrinted < intervalsShown)
                {
                    cout << "Interval [" << secondsToTime(intervalCount.first.first) << " - " << secondsToTime(intervalCount.first.second) << "]: " << intervalCount.second << " matches" << endl;
                    intervalsPrinted++;
                }
            }
        }

        messages.clear();
        secondsList.clear();
        messageFrequency.clear();
        mostUsedPhrase.clear();
        phraseCount.clear();
        intervalPhraseCounts.clear();

    }
    return 0;
}