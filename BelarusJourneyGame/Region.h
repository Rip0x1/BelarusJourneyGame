#pragma once
#include "Region.g.h"

namespace winrt::BelarusJourneyGame::implementation
{
    struct Region : RegionT<Region>
    {
        Region() = default;

        int32_t Id();
        void Id(int32_t value);

        hstring Name();
        void Name(hstring const& value);

        hstring Description();
        void Description(hstring const& value);

        hstring MapUrl();
        void MapUrl(hstring const& value);

        hstring ImagePath();
        void ImagePath(hstring const& value);

        bool IsUnlocked();
        void IsUnlocked(bool value);

        hstring LastEvent();
        void LastEvent(hstring const& value);

        int32_t ExplorationProgress();
        void ExplorationProgress(int32_t value);

        hstring SecretFact();
        void SecretFact(hstring const& value);

        double Latitude();
        void Latitude(double value);
        double Longitude();
        void Longitude(double value);

        winrt::event_token PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;

    private:
        void RaisePropertyChanged(hstring const& propertyName);

        int32_t m_id{ 0 };
        hstring m_name{ L"" };
        hstring m_description{ L"" };
        hstring m_mapUrl{ L"" };
        hstring m_imagePath{ L"" };
        hstring m_secretFact{ L"" };
        hstring m_lastEvent{ L"" };
        bool m_isUnlocked{ false };
        int32_t m_explorationProgress{ 0 };
        double m_latitude{ 0.0 };
        double m_longitude{ 0.0 };

        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}

namespace winrt::BelarusJourneyGame::factory_implementation
{
    struct Region : RegionT<Region, implementation::Region>
    {};
}