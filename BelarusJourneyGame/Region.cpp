#include "pch.h"
#include "Region.h"
#include "Region.g.cpp"

namespace winrt::BelarusJourneyGame::implementation
{
    int32_t Region::Id() { return m_id; }
    void Region::Id(int32_t value) { m_id = value; }

    hstring Region::Name() { return m_name; }
    void Region::Name(hstring const& value) { m_name = value; }

    hstring Region::Description() { return m_description; }
    void Region::Description(hstring const& value) { m_description = value; }

    hstring Region::MapUrl() { return m_mapUrl; }
    void Region::MapUrl(hstring const& value) { m_mapUrl = value; }

    hstring Region::ImagePath() { return m_imagePath; }
    void Region::ImagePath(hstring const& value) { m_imagePath = value; }

    double Region::Latitude() { return m_latitude; }
    void Region::Latitude(double value) { m_latitude = value; }

    double Region::Longitude() { return m_longitude; }
    void Region::Longitude(double value) { m_longitude = value; }

    bool Region::IsUnlocked() { return m_isUnlocked; }
    void Region::IsUnlocked(bool value) {
        if (m_isUnlocked != value) {
            m_isUnlocked = value;
            RaisePropertyChanged(L"IsUnlocked");
        }
    }

    hstring Region::SecretFact() { return m_secretFact; }
    void Region::SecretFact(hstring const& value) { m_secretFact = value; }

    int32_t Region::ExplorationProgress() { return m_explorationProgress; }
    void Region::ExplorationProgress(int32_t value) {
        if (m_explorationProgress != value) {
            m_explorationProgress = value;
            RaisePropertyChanged(L"ExplorationProgress");
        }
    }

    hstring Region::LastEvent() { return m_lastEvent; }
    void Region::LastEvent(hstring const& value) {
        if (m_lastEvent != value) {
            m_lastEvent = value;
            RaisePropertyChanged(L"LastEvent");
        }
    }

    winrt::event_token Region::PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler) {
        return m_propertyChanged.add(handler);
    }

    void Region::PropertyChanged(winrt::event_token const& token) noexcept {
        m_propertyChanged.remove(token);
    }

    void Region::RaisePropertyChanged(hstring const& propertyName) {
        m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ propertyName });
    }
}