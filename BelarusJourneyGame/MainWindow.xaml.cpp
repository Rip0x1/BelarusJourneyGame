#include "pch.h"
#include "MainWindow.xaml.h"
#include "Region.h"
#include "MainViewModel.h"
#include <Windows.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <functional>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h> 
#include <winrt/Microsoft.UI.Xaml.Controls.h> 
#include <winrt/Microsoft.UI.Text.h>
#include <winrt/Microsoft.Web.WebView2.Core.h>
#include <fstream>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <microsoft.ui.xaml.window.h>
#include <shlwapi.h>
#include <algorithm>
#include <winrt/Windows.System.h>
#pragma comment(lib, "shlwapi.lib")
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::Web::WebView2::Core;
using namespace winrt::Windows::Foundation;

std::string GetExePath() {
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(NULL, path, MAX_PATH);
	PathRemoveFileSpecW(path);
	return winrt::to_string(path) + "\\config.txt";
}

namespace winrt::BelarusJourneyGame::implementation
{
	MainWindow::MainWindow() : m_viewModel(winrt::make<winrt::BelarusJourneyGame::implementation::MainViewModel>())
	{
		InitializeComponent();
		auto windowNative{ this->try_as<::IWindowNative>() };
		HWND hWnd{ nullptr };
		if (windowNative)
		{
			windowNative->get_WindowHandle(&hWnd);
		}

		if (hWnd)
		{
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));

			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);  
		}
		MapWebView().NavigationCompleted([this](auto const&, auto const& args)
			{
				if (!args.IsSuccess())
				{
					auto status = args.WebErrorStatus();
				}
			});
		ExtendsContentIntoTitleBar(true);
		SetTitleBar(nullptr);

		auto dispatcher = this->DispatcherQueue();
		auto timer = dispatcher.CreateTimer();
		timer.Interval(std::chrono::milliseconds(1000));
		timer.IsRepeating(false);
		timer.Tick([this, timer](auto&&, auto&&)
			{
				timer.Stop();

				auto windowNative{ this->try_as<::IWindowNative>() };
				HWND hWnd{ nullptr };
				if (windowNative) windowNative->get_WindowHandle(&hWnd);

				if (hWnd)
				{
					HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
					MONITORINFO mi = { sizeof(mi) };
					mi.cbSize = sizeof(mi);

					if (GetMonitorInfo(hMonitor, &mi))
					{
						SetWindowPos(hWnd, HWND_TOP,
							mi.rcMonitor.left,
							mi.rcMonitor.top,
							mi.rcMonitor.right - mi.rcMonitor.left,
							mi.rcMonitor.bottom - mi.rcMonitor.top,
							SWP_FRAMECHANGED | SWP_SHOWWINDOW);
					}
				}
			});

		if (auto webView = MapWebView())
		{
			webView.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Left);
			webView.UpdateLayout();
			webView.HorizontalAlignment(winrt::Microsoft::UI::Xaml::HorizontalAlignment::Stretch);
		}

		timer.Start();

		this->Closed([this](auto&&, auto&&)
			{
				if (auto webView = MapWebView())
				{
					webView.Close();
				}
			});
	}

	void MainWindow::LoadMapHtml(winrt::hstring const& mapUrl)
	{
		auto webView = MapWebView();
		if (!webView || mapUrl.empty()) return;

		winrt::Windows::Foundation::Uri uri{ nullptr };
		try {
			uri = winrt::Windows::Foundation::Uri(mapUrl);
		}
		catch (...) { return; }

		if (webView.CoreWebView2())
		{
			webView.CoreWebView2().Navigate(mapUrl);
		}
		else
		{
			webView.Source(uri);
			webView.EnsureCoreWebView2Async();
		}
	}

	void MainWindow::StartGame_Click(IInspectable const&, RoutedEventArgs const&)
	{
		MenuOverlay().Visibility(Visibility::Collapsed);
		MainGameArea().Visibility(Visibility::Visible);

		auto webView = MapWebView();
		if (!webView) return;

		webView.WebMessageReceived([this](auto const&, CoreWebView2WebMessageReceivedEventArgs const& args)
			{
				try
				{
					hstring jsonRaw = args.WebMessageAsJson();
					Windows::Data::Json::JsonObject json{ nullptr };
					if (!Windows::Data::Json::JsonObject::TryParse(jsonRaw, json)) return;

					double clickLat = json.GetNamedNumber(L"lat");
					double clickLng = json.GetNamedNumber(L"lng");

					auto regions = ViewModel().Regions();
					BelarusJourneyGame::Region closestRegion = nullptr;
					double minDistance = 1000.0;

					for (auto const& r : regions)
					{
						auto region = r.as<BelarusJourneyGame::Region>();
						if (region.ExplorationProgress() >= 100) continue;

						double d = sqrt(pow(region.Latitude() - clickLat, 2) + pow(region.Longitude() - clickLng, 2));
						if (d < minDistance) {
							minDistance = d;
							closestRegion = region;
						}
					}

					if (closestRegion && minDistance < 1.5)
					{
						auto lifetime = get_strong();
						this->DispatcherQueue().TryEnqueue([this, closestRegion, regions]()
							{
								BelarusJourneyGame::Region lockedRegion = nullptr;
								for (auto const& r : regions) {
									auto reg = r.as<BelarusJourneyGame::Region>();
									if (reg.ExplorationProgress() > 0 && reg.ExplorationProgress() < 100) {
										lockedRegion = reg;
										break;
									}
								}

								bool canExplore = (lockedRegion == nullptr || closestRegion.Id() == lockedRegion.Id());

								if (canExplore)
								{
									if (ViewModel().Energy() < 15)
									{
										ViewModel().CurrentEvent(L"🚨 Топливо почти на нуле! Вы не рискнули ехать дальше.");
										EventAppearanceAnimation().Stop();
										EventAppearanceAnimation().Begin();
										return;
									}

									int cost = 10 + (rand() % 8); 
									bool badRoad = (rand() % 100 < 25); 
									if (badRoad) {
										cost += 15;
										ViewModel().CurrentEvent(L"🚜 Бездорожье! Расход топлива резко вырос.");
										EventAppearanceAnimation().Stop();
										EventAppearanceAnimation().Begin();
									}
									ViewModel().Energy(ViewModel().Energy() - cost);
									ViewModel().SelectedRegion(closestRegion);

									int chance = rand() % 100;

									if (chance < 40) 
									{
										int eventType = rand() % 3; 

										if (eventType == 0)
										{
											this->ShowEventDialog();
										}
										else if (eventType == 1)
										{
											this->StartClickMiniGame();
										}
										else
										{
											this->StartAccuracyMiniGame();
										}
									}
									else
									{
										double oldP = (double)closestRegion.ExplorationProgress();
										ViewModel().ExploreCurrentRegion();
										double newP = (double)closestRegion.ExplorationProgress();

										if (oldP != newP) {
											UpdateProgressBar(oldP, newP);
											if (oldP < 100 && newP == 100) ShowConfetti();
										}
									}

									hstring js = L"if(typeof updateRegionOnMap === 'function') { updateRegionOnMap(" +
										to_hstring(closestRegion.Latitude()) + L"," +
										to_hstring(closestRegion.Longitude()) + L", '" +
										closestRegion.Name() + L"', " +
										to_hstring(closestRegion.ExplorationProgress()) + L"); }";
									MapWebView().ExecuteScriptAsync(js);
								}
								else
								{
									ViewModel().CurrentEvent(L"Область заблокирована: " + lockedRegion.Name());
									EventAppearanceAnimation().Stop();
									EventAppearanceAnimation().Begin();
								}
							});
					}
				}
				catch (...) {}
			});

			if (webView.CoreWebView2()) {
				webView.NavigateToString(GetMapHtmlContent());

				auto token = std::make_shared<event_token>();
				*token = webView.NavigationCompleted([this, token](auto const& sender, auto const&) {
					sender.NavigationCompleted(*token); 
					this->LoadGame_Click(nullptr, nullptr); 
					});
			}
			else {
				webView.EnsureCoreWebView2Async();
				webView.CoreWebView2Initialized([this, webView](auto&&, auto&&) {
					webView.CoreWebView2().NavigateToString(GetMapHtmlContent());

					auto token = std::make_shared<event_token>();
					*token = webView.NavigationCompleted([this, token](auto const& sender, auto const&) {
						sender.NavigationCompleted(*token);
						this->LoadGame_Click(nullptr, nullptr);
						});
					});
			}
	}

	void MainWindow::SaveGame_Click(IInspectable const&, RoutedEventArgs const&)
	{
		try {
			Windows::Data::Json::JsonObject root;
			root.SetNamedValue(L"Energy", Windows::Data::Json::JsonValue::CreateNumberValue(ViewModel().Energy()));

			Windows::Data::Json::JsonArray regionsArray;
			for (auto const& item : ViewModel().Regions()) {
				auto reg = item.as<BelarusJourneyGame::Region>();
				Windows::Data::Json::JsonObject regObj;
				regObj.SetNamedValue(L"Id", Windows::Data::Json::JsonValue::CreateNumberValue(reg.Id()));
				regObj.SetNamedValue(L"Progress", Windows::Data::Json::JsonValue::CreateNumberValue(reg.ExplorationProgress()));
				regionsArray.Append(regObj);
			}
			root.SetNamedValue(L"Regions", regionsArray);

			std::ofstream file(GetExePath());
			file << winrt::to_string(root.Stringify());
			file.close();

			ViewModel().CurrentEvent(L"💾 Прогресс сохранен в профиль игрока!");
		}
		catch (...) { ViewModel().CurrentEvent(L"❌ Ошибка записи файла."); }
		EventAppearanceAnimation().Begin();
	}

	void MainWindow::LoadGame_Click(IInspectable const&, RoutedEventArgs const&)
	{
		try {
			std::ifstream file(GetExePath());
			if (!file.is_open()) {
				ViewModel().CurrentEvent(L"❓ Сохранений пока нет.");
				EventAppearanceAnimation().Begin();
				return;
			}

			std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();

			Windows::Data::Json::JsonObject root;
			if (!Windows::Data::Json::JsonObject::TryParse(winrt::to_hstring(content), root)) return;

			ViewModel().Energy((int)root.GetNamedNumber(L"Energy"));

			auto regionsArray = root.GetNamedArray(L"Regions");
			auto vmRegions = ViewModel().Regions();

			for (uint32_t i = 0; i < regionsArray.Size(); ++i) {
				auto regObj = regionsArray.GetObjectAt(i);
				int id = (int)regObj.GetNamedNumber(L"Id");
				int progress = (int)regObj.GetNamedNumber(L"Progress");

				for (auto const& item : vmRegions) {
					auto reg = item.as<BelarusJourneyGame::Region>();
					if (reg.Id() == id) {
						reg.ExplorationProgress(progress);

						if (progress > 0) {
							auto webView = MapWebView();
							if (webView && webView.CoreWebView2()) {
								hstring js = L"updateRegionOnMap(" + to_hstring(reg.Latitude()) + L"," +
									to_hstring(reg.Longitude()) + L",''," + to_hstring(progress) + L");";
								webView.ExecuteScriptAsync(js);
							}
						}
						break;
					}
				}
			}

			ViewModel().CurrentEvent(L"📂 Путешествие возобновлено!");

			if (ViewModel().SelectedRegion()) {
				UpdateProgressBar(0, (double)ViewModel().SelectedRegion().ExplorationProgress());
			}
		}
		catch (...) {
			ViewModel().CurrentEvent(L"❌ Ошибка при чтении файла.");
		}
		EventAppearanceAnimation().Begin();
	}

	void MainWindow::StartAccuracyMiniGame() {
		auto strong_this = get_strong();
		auto vm = ViewModel();
		auto reg = vm.SelectedRegion();
		if (!reg) return;

		auto gameStack = StackPanel();
		gameStack.Spacing(10);

		auto info = TextBlock();
		info.Text(L"ТУМАН! Успейте нажать на все огни за 8 секунд!");
		info.TextAlignment(TextAlignment::Center);

		auto timeBar = Microsoft::UI::Xaml::Controls::ProgressBar();
		timeBar.Maximum(80);
		timeBar.Value(80);
		timeBar.Foreground(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Orange()));

		Canvas gameCanvas;
		gameCanvas.Width(400);
		gameCanvas.Height(300);
		gameCanvas.Background(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Black()));

		gameStack.Children().Append(info);
		gameStack.Children().Append(timeBar);
		gameStack.Children().Append(gameCanvas);

		ContentDialog dialog;
		dialog.XamlRoot(this->Content().XamlRoot());
		dialog.Content(gameStack);
		dialog.Title(winrt::box_value(L"Мини-игра: Ориентирование"));

		auto left = std::make_shared<int>(10);
		auto animTimer = this->DispatcherQueue().CreateTimer();
		animTimer.Interval(std::chrono::milliseconds(100));

		animTimer.Tick([animTimer, timeBar, dialog](auto const&, auto const&) {
			double val = timeBar.Value() - 1;
			timeBar.Value(val);
			if (val <= 0) {
				animTimer.Stop();
				dialog.Hide();
			}
			});

		for (int i = 0; i < 10; i++) {
			auto circle = Microsoft::UI::Xaml::Shapes::Ellipse();
			circle.Width(35); circle.Height(35);
			circle.Fill(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Gold()));

			Canvas::SetLeft(circle, (double)(rand() % 350));
			Canvas::SetTop(circle, (double)(rand() % 250));

			circle.PointerPressed([=](auto const&, auto const&) {
				uint32_t index;
				if (gameCanvas.Children().IndexOf(circle, index)) {
					gameCanvas.Children().RemoveAt(index);
					*left -= 1;
					if (*left <= 0) {
						dialog.Hide();
					}
				}
				});
			gameCanvas.Children().Append(circle);
		}

		animTimer.Start();

		auto op = dialog.ShowAsync();

		op.Completed([strong_this, left, vm, reg, animTimer](auto&&, auto&&) {
			animTimer.Stop();
			strong_this->DispatcherQueue().TryEnqueue([strong_this, left, vm, reg]() {
				double oldP = (double)reg.ExplorationProgress();

				if (*left <= 0) {
					reg.ExplorationProgress((std::min)(100, reg.ExplorationProgress() + 20));
					vm.CurrentEvent(L"🎯 Вы прорвались сквозь туман! (+20%)");
				}
				else {
					int penalty = 15;
					reg.ExplorationProgress((std::max)(0, reg.ExplorationProgress() - penalty));
					vm.Energy((std::max)(0, vm.Energy() - 30));
					vm.CurrentEvent(L"🌫 Время вышло! Вы заблудились (-15%, -30 энергии).");
				}

				strong_this->UpdateProgressBar(oldP, (double)reg.ExplorationProgress());
				if (reg.ExplorationProgress() == 100 && oldP < 100) strong_this->ShowConfetti();

				strong_this->EventAppearanceAnimation().Stop();
				strong_this->EventAppearanceAnimation().Begin();

				hstring js = L"updateRegionOnMap(" + to_hstring(reg.Latitude()) + L"," +
					to_hstring(reg.Longitude()) + L",''," +
					to_hstring(reg.ExplorationProgress()) + L");";
				strong_this->MapWebView().ExecuteScriptAsync(js);
				});
			});
	}

	void MainWindow::UpdateProgressBar(double oldValue, double newValue)
	{
		if (oldValue == newValue) return;

		BarAnimation().From(oldValue);
		BarAnimation().To(newValue);

		ProgressBarAnimation().Stop();
		ProgressBarAnimation().Begin();
	}

	void MainWindow::RestButton_Click(IInspectable const& sender, RoutedEventArgs const&)
	{
		auto viewModel = ViewModel();

		viewModel.CurrentEvent(L"💤 Вы остановились на долгий привал (5 сек)...");
		EventAppearanceAnimation().Stop();
		EventAppearanceAnimation().Begin();

		auto btn = sender.as<Button>();
		btn.IsEnabled(false);

		auto timer = this->DispatcherQueue().CreateTimer();
		timer.Interval(std::chrono::milliseconds(5000));
		timer.IsRepeating(false);

		auto lifetime = get_strong();

		timer.Tick([this, btn, timer](auto&&, auto&&)
			{
				timer.Stop();

				auto vm = ViewModel();
				auto reg = vm.SelectedRegion();

				vm.Rest();

				if (reg && reg.ExplorationProgress() > 0 && reg.ExplorationProgress() < 100)
				{
					double oldP = (double)reg.ExplorationProgress();
					int currentP = reg.ExplorationProgress();
					int penalty = 15; 
					int newP = (currentP > penalty) ? (currentP - penalty) : 0;

					reg.ExplorationProgress(newP);
					UpdateProgressBar(oldP, (double)newP);
					vm.CurrentEvent(L"☕ Вы отдохнули, но из-за долгой стоянки часть маршрута забыта (-10% прогресса).");

					EventAppearanceAnimation().Stop();
					EventAppearanceAnimation().Begin();

					hstring js = L"if(typeof updateRegionOnMap === 'function') { updateRegionOnMap(" +
						to_hstring(reg.Latitude()) + L"," +
						to_hstring(reg.Longitude()) + L",''," +
						to_hstring(newP) + L"); }";
					MapWebView().ExecuteScriptAsync(js);
				}
				else
				{
					vm.CurrentEvent(L"🔋 Вы полны сил и готовы к новым открытиям!");
					EventAppearanceAnimation().Stop();
					EventAppearanceAnimation().Begin();
				}

				btn.IsEnabled(true);
			});

		timer.Start();
	}

	void MainWindow::ShowEventDialog()
	{
		ContentDialog dialog;
		dialog.XamlRoot(this->Content().XamlRoot());

		dialog.PrimaryButtonText(L"Принять");
		dialog.SecondaryButtonText(L"");
		dialog.DefaultButton(ContentDialogButton::Primary);

		StackPanel panel;
		panel.Spacing(15);
		panel.MinWidth(300);

		FontIcon icon;
		icon.FontFamily(Microsoft::UI::Xaml::Media::FontFamily(L"Segoe Fluent Icons"));
		icon.FontSize(50);
		icon.Foreground(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Gold()));
		icon.HorizontalAlignment(HorizontalAlignment::Center);
		icon.Margin(Thickness{ 0, 10, 0, 10 });

		TextBlock title;
		title.FontSize(20);
		title.FontWeight(Microsoft::UI::Text::FontWeights::ExtraBold());
		title.TextAlignment(TextAlignment::Center);
		title.TextWrapping(TextWrapping::Wrap);

		TextBlock description;
		description.FontSize(15);
		description.Opacity(0.8);
		description.TextAlignment(TextAlignment::Center);
		description.TextWrapping(TextWrapping::Wrap);

		int type = rand() % 7;

		if (type == 0) {
			dialog.Title(box_value(L"Местный фестиваль"));
			icon.Glyph(L"\uE701");
			title.Text(L"Вы попали на «Дожинки»!");
			description.Text(L"Местные жители делятся историями.\n\nРезультат: +10% исследования.");
			dialog.PrimaryButtonText(L"Ура!");
		}
		else if (type == 1) {
			dialog.Title(box_value(L"Заброшенная тропа"));
			icon.Glyph(L"\uE80F");
			title.Text(L"Старая усадьба");
			description.Text(L"Путь через парк усадьбы. Рискнете?\n\n70%: +20% прогресса\n30%: -15% прогресса (заблудились)");
			dialog.PrimaryButtonText(L"Рискнуть");
			dialog.SecondaryButtonText(L"В объезд");
		}
		else if (type == 2) {
			dialog.Title(box_value(L"Помощь на дороге"));
			icon.Glyph(L"\uE734");
			title.Text(L"Сломавшийся трактор");
			description.Text(L"Поделиться топливом?\n\nЦена: -15 энергии\nБонус: +25% прогресса");
			dialog.PrimaryButtonText(L"Помочь");
			dialog.SecondaryButtonText(L"Отказать");
		}
		else if (type == 3) {
			dialog.Title(box_value(L"Фото-стоп"));
			icon.Glyph(L"\uEB9F");
			title.Text(L"Золотой час");
			description.Text(L"Вид на Неман.\n\nБонус: +5 энергии и +5% прогресса.");
			dialog.PrimaryButtonText(L"Сделать фото");
		}
		else if (type == 4) { 
			dialog.Title(box_value(L"Ошибка навигации"));
			icon.Glyph(L"\uE707"); 
			title.Text(L"Дорожные работы");
			description.Text(L"Вы поехали по старой карте и уперлись в тупик.\n\nРезультат: -10% прогресса и -10 энергии.");
			dialog.PrimaryButtonText(L"Искать объезд");
		}
		else if (type == 5) { 
			dialog.Title(box_value(L"Досадный инцидент"));
			icon.Glyph(L"\uE711");
			title.Text(L"Пробитое колесо");
			description.Text(L"На гравийной дороге вы пробили колесо и потеряли время.\n\nРезультат: -20% прогресса.");
			dialog.PrimaryButtonText(L"Чинить");
		}
		else {
			dialog.Title(box_value(L"Лесная аптека"));
			icon.Glyph(L"\uE945");
			title.Text(L"Криница");
			description.Text(L"Чистый источник воды.\n\nБонус: +20 энергии.");
			dialog.PrimaryButtonText(L"Напиться");
		}

		panel.Children().Append(icon);
		panel.Children().Append(title);
		panel.Children().Append(description);
		dialog.Content(panel);

		auto op = dialog.ShowAsync();
		auto strong_this = get_strong();

		op.Completed([strong_this, type](auto const& asyncOp, AsyncStatus const& status) {
			if (status == AsyncStatus::Completed) {
				auto result = asyncOp.GetResults();
				strong_this->DispatcherQueue().TryEnqueue([strong_this, result, type]() {
					auto vm = strong_this->ViewModel();
					auto reg = vm.SelectedRegion();
					if (!reg) return;

					double oldP = (double)reg.ExplorationProgress();

					if (result == ContentDialogResult::Primary) {
						if (type == 0) {
							reg.ExplorationProgress((std::min)(100, reg.ExplorationProgress() + 10));
							vm.CurrentEvent(L"🎉 Праздник зарядил вас оптимизмом!");
						}
						else if (type == 1) { 
							if (rand() % 100 < 70) {
								reg.ExplorationProgress((std::min)(100, reg.ExplorationProgress() + 20));
								vm.CurrentEvent(L"🏰 Усадьба открыла свои тайны! (+20%)");
							}
							else {
								reg.ExplorationProgress((std::max)(0, reg.ExplorationProgress() - 15));
								vm.CurrentEvent(L"🌿 Вы заблудились в зарослях усадьбы... (-15%)");
							}
						}
						else if (type == 2) { 
							if (vm.Energy() >= 15) {
								vm.Energy(vm.Energy() - 15);
								reg.ExplorationProgress((std::min)(100, reg.ExplorationProgress() + 25));
								vm.CurrentEvent(L"🚜 Тракторист показал скрытые тропы!");
							}
							else {
								vm.CurrentEvent(L"🚨 У вас не хватило топлива, чтобы помочь.");
							}
						}
						else if (type == 3) {
							vm.Energy((std::min)(100, vm.Energy() + 5));
							reg.ExplorationProgress((std::min)(100, reg.ExplorationProgress() + 5));
							vm.CurrentEvent(L"📸 Этот кадр определенно стоил того!");
						}
						else if (type == 4) {
							reg.ExplorationProgress((std::max)(0, reg.ExplorationProgress() - 10));
							vm.Energy((std::max)(0, vm.Energy() - 10));
							vm.CurrentEvent(L"🚧 Тупик! Пришлось искать объезд.");
						}
						else if (type == 5) {
							reg.ExplorationProgress((std::max)(0, reg.ExplorationProgress() - 20));
							vm.CurrentEvent(L"🔧 Потеря времени на ремонт колеса (-20%)");
						}
						else {
							vm.Energy((std::min)(100, vm.Energy() + 20));
							vm.CurrentEvent(L"💧 Источник восстановил ваши силы.");
						}
					}
					else {
						vm.CurrentEvent(L"🛡 Вы решили не рисковать.");
					}

					strong_this->UpdateProgressBar(oldP, (double)reg.ExplorationProgress());
					if (reg.ExplorationProgress() == 100 && oldP < 100) strong_this->ShowConfetti();

					strong_this->EventAppearanceAnimation().Stop();
					strong_this->EventAppearanceAnimation().Begin();

					hstring jsCmd = L"updateRegionOnMap(" + to_hstring(reg.Latitude()) + L"," + to_hstring(reg.Longitude()) + L",''," + to_hstring(reg.ExplorationProgress()) + L");";
					strong_this->MapWebView().ExecuteScriptAsync(jsCmd);
					});
			}
			});
	}

	void MainWindow::StartClickMiniGame() {
		auto vm = ViewModel();
		auto reg = vm.SelectedRegion();
		if (!reg) return;

		auto gameStack = StackPanel();
		gameStack.Spacing(15);
		gameStack.Padding(Thickness{ 20 });
		gameStack.MinWidth(350);

		FontIcon icon;
		icon.Glyph(L"\uE9D5");
		icon.FontSize(40);
		icon.Foreground(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Orange()));

		auto infoText = TextBlock();
		infoText.Text(L"ДОРОГА РАЗМЫТА!\nПомогите машине выбраться из грязи, быстро нажимая на кнопку.");
		infoText.TextAlignment(TextAlignment::Center);
		infoText.TextWrapping(TextWrapping::Wrap);
		infoText.Opacity(0.8);

		ProgressBar timeProgress;
		timeProgress.Maximum(100);
		timeProgress.Value(100);
		timeProgress.Foreground(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::LawnGreen()));
		timeProgress.Visibility(Visibility::Collapsed);

		auto clickBtn = Button();
		clickBtn.Content(winrt::box_value(L"ПОНЯТНО, СТАРТ!"));
		clickBtn.HorizontalAlignment(HorizontalAlignment::Stretch);
		clickBtn.Height(70);
		clickBtn.FontSize(20);
		clickBtn.CornerRadius(Microsoft::UI::Xaml::CornerRadius{ 12 });
		clickBtn.Style(Microsoft::UI::Xaml::Application::Current().Resources().Lookup(winrt::box_value(L"AccentButtonStyle")).as<Style>());

		gameStack.Children().Append(icon);
		gameStack.Children().Append(infoText);
		gameStack.Children().Append(timeProgress);
		gameStack.Children().Append(clickBtn);

		ContentDialog gameDialog;
		gameDialog.XamlRoot(this->Content().XamlRoot());
		gameDialog.Title(winrt::box_value(L"Испытание: Бездорожье"));
		gameDialog.Content(gameStack);

		auto clicksNeeded = std::make_shared<int>(20);
		auto isStarted = std::make_shared<bool>(false);

		auto timer = this->DispatcherQueue().CreateTimer();
		timer.Interval(std::chrono::milliseconds(10000));

		auto animTimer = this->DispatcherQueue().CreateTimer();
		animTimer.Interval(std::chrono::milliseconds(100));

		auto strong_this = get_strong();

		clickBtn.Click([=, at = animTimer](IInspectable const&, RoutedEventArgs const&) mutable {
			if (!*isStarted) {
				*isStarted = true;
				at.Start();
				timeProgress.Visibility(Visibility::Visible);
				infoText.Text(L"ЖМИТЕ КАК МОЖНО БЫСТРЕЕ!");
				clickBtn.Content(winrt::box_value(L"КЛИКАЙ! (20)"));
				return;
			}

			if (*clicksNeeded > 0) {
				*clicksNeeded -= 1;
				clickBtn.Content(winrt::box_value(L"ОСТАЛОСЬ: " + winrt::to_hstring(*clicksNeeded)));
			}

			if (*clicksNeeded <= 0) {
				at.Stop();
				gameDialog.Hide();
			}
			});

		animTimer.Tick([=, dialog = gameDialog](auto const&, auto const&) mutable {
			double newValue = timeProgress.Value() - 1;
			timeProgress.Value(newValue);

			if (newValue < 30)
				timeProgress.Foreground(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Red()));

			if (newValue <= 0) {
				animTimer.Stop();
				dialog.Hide();
			}
			});

		auto op = gameDialog.ShowAsync();
		op.Completed([strong_this, clicksNeeded, vm, reg](auto&&, auto&&) {
			strong_this->DispatcherQueue().TryEnqueue([strong_this, clicksNeeded, vm, reg]() {
				double oldP = (double)reg.ExplorationProgress();

				if (*clicksNeeded <= 0) {
					reg.ExplorationProgress((std::min)(100, reg.ExplorationProgress() + 20));
					vm.CurrentEvent(L"🏆 Грязь позади! Вы выбрались. (+20%)");
				}
				else {
					vm.Energy((std::max)(0, vm.Energy() - 25));
					vm.CurrentEvent(L"💀 Машина застряла... (-25 энергии)");
				}

				strong_this->UpdateProgressBar(oldP, (double)reg.ExplorationProgress());

				auto anim = strong_this->EventAppearanceAnimation();
				anim.Stop();
				anim.Begin();

				hstring js = L"updateRegionOnMap(...)";
				strong_this->MapWebView().ExecuteScriptAsync(js);
				});
			});
	}

	void MainWindow::ShowConfetti()
	{
		auto canvas = ConfettiCanvas();
		double startX = canvas.ActualWidth() / 2;
		double startY = canvas.ActualHeight() / 3;

		for (int i = 0; i < 60; i++)
		{
			Microsoft::UI::Xaml::Shapes::Rectangle rect;
			rect.Width(rand() % 10 + 5);
			rect.Height(rand() % 10 + 5);

			uint8_t r = (rand() % 155) + 100;
			uint8_t g = (rand() % 155) + 100;
			rect.Fill(Microsoft::UI::Xaml::Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, r, g, 0)));

			canvas.Children().Append(rect);
			Canvas::SetLeft(rect, startX);
			Canvas::SetTop(rect, startY);

			auto storyboard = Microsoft::UI::Xaml::Media::Animation::Storyboard();

			auto animX = Microsoft::UI::Xaml::Media::Animation::DoubleAnimation();
			double targetX = startX + (rand() % 600 - 300);
			animX.To(targetX);

			winrt::Windows::Foundation::TimeSpan durationX{ std::chrono::milliseconds(1000 + rand() % 1000) };
			animX.Duration(Microsoft::UI::Xaml::DurationHelper::FromTimeSpan(durationX));

			Microsoft::UI::Xaml::Media::Animation::Storyboard::SetTarget(animX, rect);
			Microsoft::UI::Xaml::Media::Animation::Storyboard::SetTargetProperty(animX, L"(Canvas.Left)");

			auto animY = Microsoft::UI::Xaml::Media::Animation::DoubleAnimation();
			animY.To(canvas.ActualHeight() + 50);

			winrt::Windows::Foundation::TimeSpan durationY{ std::chrono::milliseconds(1500 + rand() % 1000) };
			animY.Duration(Microsoft::UI::Xaml::DurationHelper::FromTimeSpan(durationY));

			Microsoft::UI::Xaml::Media::Animation::Storyboard::SetTarget(animY, rect);
			Microsoft::UI::Xaml::Media::Animation::Storyboard::SetTargetProperty(animY, L"(Canvas.Top)");

			auto animFade = Microsoft::UI::Xaml::Media::Animation::DoubleAnimation();
			animFade.To(0.0);
			animFade.Duration(Microsoft::UI::Xaml::DurationHelper::FromTimeSpan(durationY));
			Microsoft::UI::Xaml::Media::Animation::Storyboard::SetTarget(animFade, rect);
			Microsoft::UI::Xaml::Media::Animation::Storyboard::SetTargetProperty(animFade, L"Opacity");
			storyboard.Children().Append(animFade);

			storyboard.Children().Append(animX);
			storyboard.Children().Append(animY);
			storyboard.Begin();
		}
	}

	void MainWindow::ExitButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		this->Close();
	}

	void MainWindow::RegionsList_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
	{
		auto selected = ViewModel().SelectedRegion();
		if (!selected) return;

		if (auto webView = MapWebView().CoreWebView2())
		{
			hstring js = L"if(typeof map !== 'undefined') { map.flyTo([" +
				to_hstring(selected.Latitude()) + L"," +
				to_hstring(selected.Longitude()) + L"], 8); }";

			webView.ExecuteScriptAsync(js);
		}
	}
	winrt::hstring MainWindow::GetMapHtmlContent()
	{
		return winrt::hstring(
			L"<!DOCTYPE html><html><head>"
			L"<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>"
			L"<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>"
			L"<style>body { margin: 0; padding: 0; } #map { height: 100vh; width: 100vw; background: #f0f0f0; }</style></head>"
			L"<body><div id='map'></div><script>"
			L"var map = L.map('map', { zoomControl: false, minZoom: 6, maxZoom: 10 }).setView([53.9, 27.56], 7);"
			L"L.tileLayer('https://{s}.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}{r}.png').addTo(map);"

			L"var circles = {};"
			L"var markers = {};"

			L"map.on('click', function(e) {"
			L"  window.chrome.webview.postMessage({ lat: e.latlng.lat, lng: e.latlng.lng });"
			L"});"

			L"function updateRegionOnMap(lat, lng, name, progress) {"
			L"  if (!map) return;"
			L"  var id = lat + '_' + lng;"
			L"  var isFinished = progress >= 100;"

			L"  if (!circles[id]) {"
			L"    var hue = Math.abs(Math.sin(lat + lng) * 360);"
			L"    var color = 'hsl(' + hue + ', 70%, 50%)';"

			L"    circles[id] = L.circle([lat, lng], {"
			L"      radius: 5000,"
			L"      color: color,"
			L"      fillColor: color,"
			L"      fillOpacity: 0.4,"
			L"      weight: 2,"
			L"      interactive: true"
			L"    }).addTo(map);"

			L"    markers[id] = L.marker([lat, lng], { interactive: false }).addTo(map);"

			L"    circles[id].on('click', function(e) {"
			L"      window.chrome.webview.postMessage({ lat: lat, lng: lng });"
			L"      L.DomEvent.stopPropagation(e);"
			L"    });"
			L"  }"

			L"  var currentRadius = 5000 + (progress * 150);"
			L"  var opacity = isFinished ? 0.15 : 0.4;"

			L"  circles[id].setRadius(currentRadius);"
			L"  circles[id].setStyle({"
			L"    fillOpacity: opacity,"
			L"    opacity: opacity,"
			L"    interactive: !isFinished"
			L"  });"

			L"  if (isFinished) {"
			L"    circles[id].bringToBack();"
			L"    markers[id].setOpacity(0.6);"
			L"  }"
			L"}"
			L"</script></body></html>"
		);
	}

	void MainWindow::CloseWebView()
	{
		if (MapWebView())
		{
			MapWebView().Close();
		}
	}

	BelarusJourneyGame::MainViewModel MainWindow::ViewModel()
	{
		return m_viewModel;
	}
}