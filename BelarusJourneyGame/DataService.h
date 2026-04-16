#pragma once
#include <string>
#include <vector>

namespace BelarusJourneyGame::Models {
    struct RegionData {
        int Id;
        std::wstring Name;
        std::wstring Description;
        std::wstring MapUrl;
        std::wstring ImagePath;
        std::wstring SecretFact;
        double Latitude;
        double Longitude;
    };
}

namespace BelarusJourneyGame::Services {
    class DataService {
    public:
        static std::vector<BelarusJourneyGame::Models::RegionData> GetBelarusRegions();
    };
}