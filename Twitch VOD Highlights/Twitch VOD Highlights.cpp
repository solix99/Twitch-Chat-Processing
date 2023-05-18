#define INTERVAL_DURATION 10
#define HIGHLIGHT_STRENGTH_FACTOR 2

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <unordered_map>

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

struct YourHashFunction {
    size_t operator()(const string& str) const {
        size_t hash = 0;
        for (const char c : str) {
            hash = (hash * 131) + c; // Modify the prime number as needed
        }
        return hash;
    }
};

int main()
{
    string fileName, line;
    double HSF = HIGHLIGHT_STRENGTH_FACTOR;
    double intervalDuration = INTERVAL_DURATION;

    while (true)
    {
        string searchMessage;
        int dec, intervalsShown;

       // cout << "1-Message frequency search" << endl;
       // cout << "2-Exact message search" << endl;
       // cin >> dec;

        dec = 1;

        if (dec == 1)
        {
            cout << "Enter VOD ID, highlight strength factor, and interval duration: ";
            cin >> fileName >> HSF >> intervalDuration;
        }
        else if (dec == 0)
        {
            cout << "Enter VOD ID, interval duration, exact message, intervals shown: ";
            cin >> fileName >> intervalDuration >> searchMessage >> intervalsShown;
        }

        string fileDirectory = fileName + ".txt";
        ifstream inFile(fileDirectory);

        if (!inFile)
        {
            cout << "File not found." << endl;
            continue;
        }

        getline(inFile, line);
        line = line.substr(3);

        vector<double> secondsList;
        vector<string> messages;
        int hours, minutes, seconds;

        while (getline(inFile, line))
        {
            int result = sscanf_s(line.c_str(), "[%lld:%lld:%lld]", &hours, &minutes, &seconds);
            size_t pos1 = line.find(' ') + 1;
            size_t pos2 = line.find(':', pos1);
            string username = line.substr(pos1, pos2 - pos1);
            string message = line.substr(pos2 + 2);
            transform(message.begin(), message.end(), message.begin(), ::tolower);
            double totalSeconds = hours * 3600 + minutes * 60 + seconds;

            secondsList.push_back(totalSeconds);
            messages.push_back(move(message));
        }

        int numMessages = secondsList.size();
        int numIntervals = ceil(secondsList.back() / intervalDuration);
        vector<int> messageFrequency(numIntervals, 0);
        vector<string> mostUsedPhrase(numIntervals);
        vector<int> phraseCount(numIntervals);
        vector<unordered_map<string, int, YourHashFunction>> intervalPhraseCounts(numIntervals);

        int currentInterval = 0;

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
                phraseCountMap[word]++;
            }

            int currentPhraseCount = phraseCountMap[currentPhrase];
            int& mostUsedPhraseCount = phraseCount[currentInterval];

            if (currentPhraseCount > mostUsedPhraseCount)
            {
                mostUsedPhraseCount = currentPhraseCount;
                mostUsedPhrase[currentInterval] = currentPhrase;
            }
        }

        inFile.close();

        if (dec == 1)
        {
            int numMessages = secondsList.size();
            double averageFrequency = numMessages / secondsList.back();

            for (int i = 0; i < numIntervals; i++)
            {
                double intervalStart = i * intervalDuration;
                double intervalEnd = min((i + 1) * intervalDuration - 1, secondsList.back());

                if (messageFrequency[i] > (averageFrequency * intervalDuration) * HSF)
                {
                    cout << endl << "Interval [" << secondsToTime(intervalStart) << " - " << secondsToTime(intervalEnd) << "]: " << messageFrequency[i] << " messages " << mostUsedPhrase[i].substr(0, 10) << "[" << phraseCount[i] << "]" << endl;
                }
            }
        }
        else if (dec == 0)
        {
            vector<pair<pair<int, int>, int>> intervalCounts;

            for (int i = 0; i < numIntervals; i++)
            {
                int intervalStart = i * intervalDuration;
                int intervalEnd = min((i + 1) * intervalDuration - 1, secondsList.back());
                int messageCount = 0;

                for (int j = 0; j < numMessages; j++)
                {
                    if (secondsList[j] >= intervalStart && secondsList[j] <= intervalEnd)
                    {
                        string lowerMessage = messages[j];
                        transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);
                        string lowerSearchMessage = searchMessage;
                        transform(lowerSearchMessage.begin(), lowerSearchMessage.end(), lowerSearchMessage.begin(), ::tolower);
                        size_t pos = lowerMessage.find(lowerSearchMessage, 0);

                        while (pos != string::npos)
                        {
                            messageCount++;
                            pos = lowerMessage.find(lowerSearchMessage, pos + 1);
                        }
                    }
                }

                intervalCounts.push_back(make_pair(make_pair(intervalStart, intervalEnd), messageCount));
            }

            sort(intervalCounts.begin(), intervalCounts.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.second > rhs.second;
                });

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
