#include <iostream>
#include <cstring>
#include <map>
#include <chrono>
#include <unordered_map>
#include <fstream>
#include <thread>
#include <string>
#include <sstream> 
#include <iomanip>
#include <semaphore>
#include <thread>
#include <atomic>
#include <memory_resource>
#include <tuple>

constexpr uint8_t ARG_MAX = 10;

std::vector<std::string> splitSentencesIntoWords(const std::vector<std::string>& sentences);
std::tuple<size_t, std::vector<std::string>, std::vector<size_t>> getWordCount(size_t timestamp, const std::vector<std::string>& wordData);

struct Segment
{
public:
    static void newSegment(std::string& pUsername, std::string& pMessage, unsigned int timestamp)
    {
        _messageMap[timestamp].push_back(pMessage);
        _WORD_MAP[timestamp].push_back(pMessage);
        _userLogsMap[pUsername].push_back(pMessage);
        _freqMap[timestamp]++;

        if (timestamp > _LAST_TIMESTAMP)
        {
            _WORD_MAP[_LAST_TIMESTAMP] = splitSentencesIntoWords(_messageMap[_LAST_TIMESTAMP]);

            _WORD_COUNT_VECTOR.push_back(getWordCount(timestamp, _WORD_MAP[_LAST_TIMESTAMP]));

            _LAST_TIMESTAMP = timestamp;

        }
        _SEGMENT_COUNT++;
    }

    static size_t getSegmentCount() { return _SEGMENT_COUNT; }
    std::vector<std::string>& getUserMessages(std::string& key) { return _userLogsMap[key]; }
    static void printFreqData(int INTERVAL_DURATION, float FREQ_MULITPLIER);
    static bool cleanUp();
    static bool searchData();
    static bool showLogs();

private:
    static size_t _LAST_TIMESTAMP;
    static size_t _SEGMENT_COUNT;
    static std::unordered_map<std::string, std::vector<std::string>> _userLogsMap; //USERNAME   MESSAGE_VECTOR
    static std::unordered_map<size_t, size_t > _freqMap; // TIMESTAMP   AMMOUNT_OF_MESSAGES
    static std::unordered_map<size_t, std::vector<std::string>> _messageMap; //TIMESTAMP   MESSAGES_VECTOR
    static std::unordered_map <size_t, std::vector<std::string>> _WORD_MAP;
    static std::vector<std::tuple <size_t, std::vector<std::string>, std::vector<size_t>>> _WORD_COUNT_VECTOR; // TIMESTAMP STRING COUNT_VEC
};
size_t Segment::_LAST_TIMESTAMP = 0;
size_t Segment::_SEGMENT_COUNT = 0;
std::unordered_map<std::string, std::vector<std::string>> Segment::_userLogsMap;
std::unordered_map<size_t, size_t > Segment::_freqMap;
std::unordered_map<size_t, std::vector<std::string>> Segment::_messageMap;
std::unordered_map <size_t, std::vector<std::string>> Segment::_WORD_MAP;
std::vector<std::tuple <size_t, std::vector<std::string>, std::vector<size_t>>> Segment::_WORD_COUNT_VECTOR;
bool Segment::cleanUp()
{
    for (auto& pair : _userLogsMap)
    {
        pair.second.clear();
    }
    for (auto& pair : _WORD_MAP)
    {
        pair.second.clear();
    }
    for (auto& tuple : _WORD_COUNT_VECTOR)
    {
        std::get<1>(tuple).clear(); // Clears the vector of strings
        std::get<2>(tuple).clear(); // Clears the vector of size_t
    }
    _userLogsMap.clear();
    _freqMap.clear();
    _messageMap.clear();
    _WORD_MAP.clear();
    _WORD_COUNT_VECTOR.clear();

    _LAST_TIMESTAMP = 0;
    _SEGMENT_COUNT = 0;

    return true;
}


struct Data
{
    size_t HOURS, MINUTES, SECONDS, POS_FIRST, POS_SECOND;
    size_t PHRASE_LIMIT = 10;
    size_t VOD_LENGTH = 0;

    std::vector<std::string> ARG_VEC;
    std::fstream INPUT_FILE_STREAM;
    std::string INPUT_LINE, INPUT_FILE_NAME;

    std::chrono::high_resolution_clock::time_point TIMER_START_TIME;
    int ARG_COUNT, INTERVAL_COUNT, AVERAGE_FREQUENCY;


}MEM;

std::tuple<size_t, std::vector<std::string>, std::vector<size_t>> getWordCount(size_t timestamp, const std::vector<std::string>& wordData)
{
    std::unordered_map<std::string, size_t> wordCount;
    std::vector<size_t> countResult;
    std::vector<std::string> stringResult;

    for (size_t i = 0; i < wordData.size(); i++)
    {
        //  std::cout << wordData[i] << " ";
        wordCount[wordData[i]]++;
    }
    for (const auto& pair : wordCount)
    {
        stringResult.push_back(pair.first);
        //std::cout << pair.first << " ";
        countResult.push_back(pair.second);
        // std::cout << pair.second << " ";
    }

    std::tuple<size_t, std::vector<std::string>, std::vector<size_t>> tupleResult = std::make_tuple(timestamp, stringResult, countResult);
    //std::cout << std::endl << std::get<0>(tupleResult);
    return tupleResult;
}

std::vector<std::string> mergeStringVectors(std::vector<std::string>& vector1, const std::vector<std::string>& vector2)
{
    vector1.reserve(vector1.size() + vector2.size());

    vector1.insert(vector1.end(), vector2.begin(), vector2.end());

    return vector1;
}

std::vector<size_t> mergeIntVectors(const std::vector<size_t>& vector1, const std::vector<size_t>& vector2)
{
    std::vector<size_t> mergedVector;
    mergedVector.reserve(vector1.size() + vector2.size()); // Preallocate memory for efficiency

    // Append elements from vector1
    mergedVector.insert(mergedVector.end(), vector1.begin(), vector1.end());

    // Append elements from vector2
    mergedVector.insert(mergedVector.end(), vector2.begin(), vector2.end());

    return mergedVector;
}


void combineAndSortCounts(std::vector<std::string>& strings, std::vector<size_t>& counts)
{
    // Create a map to store the combined counts for each string
    std::unordered_map<std::string, size_t> combinedCounts;

    // Combine counts in a more efficient way using a range-based for loop
    for (size_t i = 0; i < strings.size(); ++i) {
        combinedCounts[strings[i]] += counts[i];
    }

    // Clear the original vectors (no need to clear and rebuild them)
    strings.clear();
    counts.clear();

    // Reserve space in vectors to minimize reallocation
    strings.reserve(combinedCounts.size());
    counts.reserve(combinedCounts.size());

    // Populate the vectors with the combined counts directly
    for (const auto& pair : combinedCounts) {
        strings.push_back(pair.first);
        counts.push_back(pair.second);
    }

    // Sort the vectors in descending order based on counts
    std::vector<std::pair<std::string, size_t>> stringCountPairs;
    stringCountPairs.reserve(strings.size()); // Reserve space

    // Use a lambda for the sorting comparison
    auto compare = [](const auto& a, const auto& b) {
        return a.second > b.second; // Sort in descending order of counts
    };

    for (size_t i = 0; i < strings.size(); ++i) {
        stringCountPairs.emplace_back(strings[i], counts[i]);
    }

    std::sort(stringCountPairs.begin(), stringCountPairs.end(), compare);

    // Update the original vectors with the sorted values
    for (size_t i = 0; i < strings.size(); ++i)
    {
        strings[i] = stringCountPairs[i].first;
        counts[i] = stringCountPairs[i].second;
    }
}

void mergeTupleVector(std::vector <std::tuple<size_t, std::vector<std::string>, std::vector<size_t>>>& tupleVec, size_t INTERVAL_LENGTH) {
    // Sort the tupleVec based on the first element of each tuple (assuming it's a timestamp)
    std::sort(tupleVec.begin(), tupleVec.end(), [](const auto& a, const auto& b) {
        return std::get<0>(a) < std::get<0>(b);
        });

    size_t CURRENT_INTERVAL = 0;
    size_t currentTimestamp = std::get<0>(tupleVec[0]);

    for (size_t i = 1; i < tupleVec.size(); ++i) {
        if (std::get<0>(tupleVec[i]) > INTERVAL_LENGTH * (CURRENT_INTERVAL + 1)) {
            // Merge the data for the current interval
            tupleVec[CURRENT_INTERVAL] = std::make_tuple(CURRENT_INTERVAL,
                mergeStringVectors(std::get<1>(tupleVec[CURRENT_INTERVAL]), std::get<1>(tupleVec[i])),
                mergeIntVectors(std::get<2>(tupleVec[CURRENT_INTERVAL]), std::get<2>(tupleVec[i])));

            // Update the current interval and clear the data in the merged interval
            ++CURRENT_INTERVAL;
            tupleVec[CURRENT_INTERVAL] = std::make_tuple(std::get<0>(tupleVec[i]), std::vector<std::string>(), std::vector<size_t>());
        }
        else {
            // Continue merging data into the current interval
            std::get<1>(tupleVec[CURRENT_INTERVAL]) = mergeStringVectors(std::get<1>(tupleVec[CURRENT_INTERVAL]), std::get<1>(tupleVec[i]));
            std::get<2>(tupleVec[CURRENT_INTERVAL]) = mergeIntVectors(std::get<2>(tupleVec[CURRENT_INTERVAL]), std::get<2>(tupleVec[i]));
        }
    }

    // Merge and sort the final intervals
    for (size_t i = 0; i <= CURRENT_INTERVAL; ++i) {
        combineAndSortCounts(std::get<1>(tupleVec[i]), std::get<2>(tupleVec[i]));
    }
}

bool isInteger(const std::string& str)
{

    try
    {
        std::stoi(str);
        return true; // Successful conversion means it's an integer
    }
    catch (const std::invalid_argument&)
    {
        return false; // Conversion failed, not an integer
    }
    catch (const std::out_of_range&)
    {
        return false; // Conversion resulted in out-of-range value
    }
}

inline bool fileExists(const std::string& name)
{
    std::ifstream f(name.c_str());
    return f.good();
}

void startTimer()
{
    MEM.TIMER_START_TIME = std::chrono::high_resolution_clock::now();
}

void stopTimer(const std::string&& info)
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - MEM.TIMER_START_TIME).count();
    std::cout << std::endl << "Task took:" << duration << " ms." << "[" << info << "]" << std::endl;
}

std::string secondsToTime(int totalSeconds)
{
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    std::stringstream ss;
    ss << "[" << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds << "]";

    return ss.str();
}

bool getArgs(const std::string& line)
{
    MEM.ARG_VEC.clear();
    std::istringstream iss(line);
    std::string word;
    iss >> word;


   
    MEM.INPUT_FILE_NAME = word + ".txt";
    

    while (iss >> word)
    {
        MEM.ARG_VEC.push_back(word);
    }

    return true;
}

std::string toLowerCase(const std::string& input) {
    std::string result;
    for (char c : input) {
        result += std::tolower(c);
    }
    return result;
}

std::vector<std::string> splitSentencesIntoWords(const std::vector<std::string>& sentences)
{
    std::vector<std::string> words;

    for (const std::string& sentence : sentences)
    {
        std::istringstream iss(sentence);
        std::string word;
        while (iss >> word)
        {
            words.push_back(word);
        }
    }
    return words;
}

void Segment::printFreqData(int INTERVAL_DURATION, float FREQ_MULITPLIER)
{
    startTimer();

    MEM.INTERVAL_COUNT = MEM.VOD_LENGTH / INTERVAL_DURATION;
    std::vector<int> INTERVAL_FREQ_VEC(MEM.INTERVAL_COUNT);

    size_t LAST_HOUR = 1, SUM = 0;

    for (size_t i = 1; i < MEM.VOD_LENGTH - INTERVAL_DURATION; i++)
    {
        if (_freqMap[i] != 0) INTERVAL_FREQ_VEC[i / INTERVAL_DURATION] += _freqMap[i];
    }

    for (size_t i = 0; i < MEM.INTERVAL_COUNT; i++)
    {
        SUM += INTERVAL_FREQ_VEC[i];
    }

    MEM.AVERAGE_FREQUENCY = SUM / MEM.INTERVAL_COUNT;

    std::cout << std::endl << "AVG FREQ:" << MEM.AVERAGE_FREQUENCY;

    mergeTupleVector(_WORD_COUNT_VECTOR, INTERVAL_DURATION);

    for (size_t i = 0; i < MEM.INTERVAL_COUNT; i++)
    {
        if (INTERVAL_FREQ_VEC[i] > MEM.AVERAGE_FREQUENCY * FREQ_MULITPLIER)
        {
            std::cout << std::endl << secondsToTime(i * INTERVAL_DURATION) << " " << INTERVAL_FREQ_VEC[i] << " messages";

            for (size_t a = 0; a < MEM.PHRASE_LIMIT; a++)
            {
                std::cout << " " << std::get<1>(_WORD_COUNT_VECTOR[i])[a] << "[" << std::get<2>(_WORD_COUNT_VECTOR[i])[a] << "]";
            }
        }
        if (i * INTERVAL_DURATION > (LAST_HOUR * 3600) - INTERVAL_DURATION + 1) { LAST_HOUR++; std::cout << std::endl; }
    }

    stopTimer("CALC FREQ+PHRASES");

}
bool Segment::searchData()
{
    startTimer();

    std::string target = MEM.ARG_VEC[1];
    for (size_t i = 0; i < _WORD_COUNT_VECTOR.size(); i++)
    {
        auto& wordVector = std::get<1>(_WORD_COUNT_VECTOR[i]);
        if (std::find(wordVector.begin(), wordVector.end(), target) != wordVector.end())
        {
            for (std::string& str2 : _messageMap[std::get<0>(_WORD_COUNT_VECTOR[i - 1])])
            {
                std::cout << std::endl << secondsToTime(std::get<0>(_WORD_COUNT_VECTOR[i - 1])) << str2;
            }
            std::cout << std::endl;
        }
    }

    stopTimer("SEARCH COMPLETE");

    return 1;
}
bool Segment::showLogs()
{
    for (std::string& str : _userLogsMap[MEM.ARG_VEC[1]])
    {
        std::cout << std::endl << MEM.ARG_VEC[1] << ": " << str;
    }

    return 1;
}

int main()
{
    while (true)
    {
        std::getline(std::cin, MEM.INPUT_LINE);

        getArgs(MEM.INPUT_LINE);

        if (!fileExists(MEM.INPUT_FILE_NAME))
        {
            std::ofstream batch_file;
            batch_file.open("commands.cmd", std::ios::trunc);
            batch_file << "TwitchDownloaderCLI.exe chatdownload --id " << MEM.INPUT_FILE_NAME.substr(0, MEM.INPUT_FILE_NAME.length() - 4) << " -o " << MEM.INPUT_FILE_NAME << std::endl;
            batch_file.close();

            std::cout << std::endl << batch_file._Stdstr;

            int batch_exit_code = system("cmd.exe /c commands.cmd"); // blocks until the child process is terminated
            if (batch_exit_code != 0) {
                std::cout << "Batch file exited with code " << batch_exit_code << std::endl;
            }

            remove("commands.cmd"); // delete the batch file
        }

        startTimer();


        MEM.INPUT_FILE_STREAM.open(MEM.INPUT_FILE_NAME);

        while (std::getline(MEM.INPUT_FILE_STREAM, MEM.INPUT_LINE))
        {
            int result = sscanf_s(MEM.INPUT_LINE.c_str(), "[%lld:%lld:%lld]", &MEM.HOURS, &MEM.MINUTES, &MEM.SECONDS);

            // Extract username and message
            MEM.POS_FIRST = MEM.INPUT_LINE.find(' ') + 1; // position of first space after timestamp
            MEM.POS_SECOND = MEM.INPUT_LINE.find(':', MEM.POS_FIRST); // position of colon after username
            std::string username = MEM.INPUT_LINE.substr(MEM.POS_FIRST, MEM.POS_SECOND - MEM.POS_FIRST);
            std::string message = MEM.INPUT_LINE.substr(MEM.POS_SECOND + 2); // position of first character of message

            Segment::newSegment(username, message, (MEM.HOURS * 3600 + MEM.MINUTES * 60 + MEM.SECONDS));

        }
        MEM.INPUT_FILE_STREAM.close();

        MEM.VOD_LENGTH = (MEM.HOURS * 3600 + MEM.MINUTES * 60 + MEM.SECONDS);

        stopTimer("LOAD FILE");

        std::cout << "Segments Created:" << Segment::getSegmentCount() << std::endl;

        if (MEM.ARG_VEC[0] == "d")
        {
            Segment::printFreqData(60, 1.5f);
        }
        else if (MEM.ARG_VEC[0] == "c")
        {
            Segment::printFreqData(std::stoi(MEM.ARG_VEC[2]), std::stof(MEM.ARG_VEC[1]));
        }
        else if (MEM.ARG_VEC[0] == "s")
        {
            Segment::searchData();
        }
        else if (MEM.ARG_VEC[0] == "l")
        {
            Segment::showLogs();
        }

        Segment::cleanUp();
    }


    return 0;
}
