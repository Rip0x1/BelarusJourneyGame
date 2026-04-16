#pragma once
#include "MainViewModel.g.h"
#include "Region.h"

namespace winrt::BelarusJourneyGame::implementation
{
    struct MainViewModel : MainViewModelT<MainViewModel>
    {
        MainViewModel();

        int32_t Energy() { return m_energy; }
        void Energy(int32_t value);
        void Rest();
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> Regions() { return m_regions; }

        winrt::BelarusJourneyGame::Region SelectedRegion() { return m_selectedRegion; }
        void SelectedRegion(winrt::BelarusJourneyGame::Region const& value);
        winrt::hstring CurrentRegionDescription() { return m_currentRegionDescription; }

        void ExploreCurrentRegion();
        winrt::hstring CurrentEvent();
        void CurrentEvent(winrt::hstring const& value);

        winrt::event_token PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;
        winrt::hstring PlayerRank();

    private:
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> m_regions{ nullptr };
        winrt::BelarusJourneyGame::Region m_selectedRegion{ nullptr };
        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        winrt::hstring m_currentRegionDescription;
        winrt::hstring m_currentEvent = L"Путешествие начинается! Выберите регион и нажмите «Исследовать».";
        int32_t m_energy{ 100 };

        void RaisePropertyChanged(winrt::hstring const& propertyName)
        {
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(propertyName));
        }
    };
}

namespace winrt::BelarusJourneyGame::factory_implementation
{
    struct MainViewModel : MainViewModelT<MainViewModel, implementation::MainViewModel>
    {};
}