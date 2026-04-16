#include "pch.h"
#include "MainViewModel.h"
#include "Region.h"
#include "DataService.h"
#include "MainViewModel.g.cpp" 
#include <winrt/Microsoft.UI.Xaml.Interop.h>

namespace winrt::BelarusJourneyGame::implementation
{
    MainViewModel::MainViewModel()
    {
        m_regions = winrt::single_threaded_observable_vector<winrt::Windows::Foundation::IInspectable>();
        auto rawData = ::BelarusJourneyGame::Services::DataService::GetBelarusRegions();

        for (auto const& item : rawData)
        {
            auto r = winrt::make<winrt::BelarusJourneyGame::implementation::Region>();

            r.Id(item.Id);
            r.Name(winrt::hstring(item.Name));
            r.Description(winrt::hstring(item.Description));
            r.SecretFact(winrt::hstring(item.SecretFact));

            r.Latitude(item.Latitude);
            r.Longitude(item.Longitude);

            r.MapUrl(winrt::hstring(item.MapUrl));

            r.ImagePath(winrt::hstring(item.ImagePath));

            m_regions.Append(r);    
        }
        m_selectedRegion = nullptr;
        m_currentEvent = L"Выберите область на карте, чтобы начать путешествие!";
        m_currentRegionDescription = L"Информация о локации появится после выбора области на карте.";
        if (m_regions.Size() > 0) {
            auto firstRegion = m_regions.GetAt(0).as<BelarusJourneyGame::Region>();
            SelectedRegion(firstRegion);
        }
    }

    winrt::event_token MainViewModel::PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void MainViewModel::PropertyChanged(winrt::event_token const& token) noexcept
    {
        m_propertyChanged.remove(token);
    }

    void MainViewModel::SelectedRegion(winrt::BelarusJourneyGame::Region const& value)
    {
        if (m_selectedRegion != value)
        {
            m_selectedRegion = value;

            if (m_selectedRegion)
            {
                winrt::hstring fullDescription = m_selectedRegion.Description();
                if (m_selectedRegion.ExplorationProgress() >= 100)
                {
                    fullDescription = fullDescription + L"\n\n🌟 СЕКРЕТНЫЙ ФАКТ:\n" + m_selectedRegion.SecretFact();
                }
                m_currentRegionDescription = fullDescription;

                winrt::BelarusJourneyGame::Region active = nullptr;
                for (auto const& r : m_regions) {
                    auto reg = r.as<winrt::BelarusJourneyGame::Region>();
                    if (reg.ExplorationProgress() > 0 && reg.ExplorationProgress() < 100) {
                        active = reg;
                        break;
                    }
                }

                winrt::hstring history = m_selectedRegion.LastEvent();

                if (active && m_selectedRegion.Id() != active.Id() && m_selectedRegion.ExplorationProgress() < 100) {
                    m_currentEvent = L"Регион заблокирован. Исследуется: " + active.Name();
                }
                else if (!history.empty()) {
                    m_currentEvent = history;
                }
                else {
                    m_currentEvent = L"Вы прибыли в " + m_selectedRegion.Name() + L". Нажмите на карту.";
                }
            }
            else {
                m_currentRegionDescription = L"Выберите область для информации.";
                m_currentEvent = L"Путешествие начинается!";
            }

            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"SelectedRegion" });
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"CurrentEvent" });
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"CurrentRegionDescription" });
        }
    }

    void MainViewModel::ExploreCurrentRegion()
    {
        auto region = SelectedRegion();
        if (!region) return;

        int chance = rand() % 100 + 1;
        winrt::hstring eventMessage;
        int progressBonus = 0;

        if (chance > 90) progressBonus = 20;
        else if (chance > 60) progressBonus = 10;
        else if (chance > 20) progressBonus = 5;
        else progressBonus = 0;

        int oldProgress = region.ExplorationProgress();
        int newProgress = oldProgress + progressBonus;
        if (newProgress > 100) newProgress = 100;

        if (oldProgress < 100 && newProgress == 100) {
            eventMessage = L"🎉 Исследование завершено! Новый факт добавлен в описание локации.";
        }
        else if (newProgress == 100) {
            eventMessage = L"Область полностью изучена.";
        }
        else {
            if (chance > 90) eventMessage = L"Вы нашли древний клад! +20%.";
            else if (chance > 60) eventMessage = L"Местный житель рассказал легенду. +10%.";
            else if (chance > 20) eventMessage = L"Вы изучили окрестности. +5%.";
            else eventMessage = L"Ничего интересного не найдено.";
        }

        region.ExplorationProgress(newProgress);
        region.LastEvent(eventMessage);
        this->CurrentEvent(eventMessage);

        if (newProgress >= 100)
        {
            m_currentRegionDescription = region.Description() + L"\n\n🌟 СЕКРЕТНЫЙ ФАКТ:\n" + region.SecretFact();
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"CurrentRegionDescription" });
        }

        m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"CurrentEvent" });
        m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"PlayerRank" });
    }

    winrt::hstring MainViewModel::PlayerRank()
    {
        int totalProgress = 0;

        for (auto const& item : m_regions)
        {
            auto region = item.as<winrt::BelarusJourneyGame::Region>();
            totalProgress += region.ExplorationProgress();
        }

        if (totalProgress >= 600) return L"Легендарный Исследователь";
        if (totalProgress >= 300) return L"Бывалый Путешественник";
        if (totalProgress >= 100) return L"Краевед";

        return L"Новичок";
    }

    void MainViewModel::Energy(int32_t value)
    {
        if (m_energy != value)
        {
            m_energy = std::clamp(value, 0, 100);
            RaisePropertyChanged(L"Energy");
        }
    }

    void MainViewModel::Rest()
    {
        Energy(100);
        this->CurrentEvent(L"Вы хорошо отдохнули. Энергия восстановлена!");
    }

    void MainViewModel::CurrentEvent(winrt::hstring const& value)
    {
        if (m_currentEvent != value)
        {
            m_currentEvent = value;
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"CurrentEvent" });
        }
    }

    winrt::hstring MainViewModel::CurrentEvent()
    {
        return m_currentEvent;
    }

}