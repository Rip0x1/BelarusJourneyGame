#pragma once
#include "MainWindow.g.h"
#include "MainViewModel.h"

namespace winrt::BelarusJourneyGame::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        BelarusJourneyGame::MainViewModel ViewModel();
        void RegionsList_SelectionChanged(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void CloseWebView();
        void ExitButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void StartGame_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void RestButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void LoadMapHtml(winrt::hstring const& mapUrl);
        void ShowConfetti();
        void ShowEventDialog();
        void StartClickMiniGame();
        void UpdateProgressBar(double oldValue, double newValue);
        void SaveGame_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void LoadGame_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void StartAccuracyMiniGame();

        static Microsoft::UI::Xaml::Visibility ConvertProgressToVisibility(int32_t progress)
        {
            return (progress >= 100) ?
                Microsoft::UI::Xaml::Visibility::Visible :
                Microsoft::UI::Xaml::Visibility::Collapsed;
        }   
        static double ProgressToOpacity(int32_t progress) { return (progress > 0) ? 1.0 : 0.5; }
        static winrt::hstring ProgressToStatusText(int32_t progress) { return (progress > 0) ? L"ИССЛЕДОВАНО" : L"НЕИССЛЕДОВАНО"; }
        static winrt::Microsoft::UI::Xaml::Media::SolidColorBrush ProgressToStatusColor(int32_t progress) {
            return (progress > 0)
                ? winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::LawnGreen())
                : winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Gray());
        }

    private:
        BelarusJourneyGame::MainViewModel m_viewModel{ nullptr };
        winrt::hstring GetMapHtmlContent();
    };
}

namespace winrt::BelarusJourneyGame::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {};
}