//
//      menu_state_join_game.cpp:
//
//      This file is part of ZetaGlest <https://github.com/ZetaGlest>
//
//      Copyright (C) 2018  The ZetaGlest team
//
//      ZetaGlest is a fork of MegaGlest <https://megaglest.org>
//
//      This program is free software: you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation, either version 3 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program.  If not, see <https://www.gnu.org/licenses/>```

#include "menu_state_join_game.h"

#include "menu_state_connected_game.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_custom_game.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "client_interface.h"
#include "conversion.h"
#include "game.h"
#include "string_utils.h"
#include "socket.h"

#include "leak_dumper.h"

namespace Glest {
	namespace Game {

		using namespace::Shared::Util;

		// ===============================
		//      class MenuStateJoinGame
		// ===============================

		const int MenuStateJoinGame::newServerIndex = 0;
		const int MenuStateJoinGame::newPrevServerIndex = 1;
		const int MenuStateJoinGame::foundServersIndex = 2;

		MenuStateJoinGame::MenuStateJoinGame(Program * program,
			MainMenu * mainMenu,
			bool *
			autoFindHost) :MenuState(program,
				mainMenu,
				"join-game") {
			CommonInit(false, Ip(), -1);

			if (autoFindHost != NULL && *autoFindHost == true) {
				//if(clientInterface->isConnected() == false) {
				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);

				buttonAutoFindServers.setEnabled(false);
				buttonConnect.setEnabled(false);

				NetworkManager & networkManager = NetworkManager::getInstance();
				ClientInterface *clientInterface =
					networkManager.getClientInterface();
				clientInterface->discoverServers(this);
				//}
			}
		}
		MenuStateJoinGame::MenuStateJoinGame(Program * program,
			MainMenu * mainMenu, bool connect,
			Ip serverIp,
			int
			portNumberOverride) :MenuState
			(program, mainMenu, "join-game") {
			CommonInit(connect, serverIp, portNumberOverride);
		}

		void MenuStateJoinGame::CommonInit(bool connect, Ip serverIp,
			int portNumberOverride) {
			containerName = "JoinGame";
			abortAutoFind = false;
			autoConnectToServer = false;
			Lang & lang = Lang::getInstance();
			Config & config = Config::getInstance();
			NetworkManager & networkManager = NetworkManager::getInstance();
			networkManager.end();
			networkManager.init(nrClient);

			string serverListPath = config.getString("ServerListPath", "");
			if (serverListPath != "") {
				endPathWithSlash(serverListPath);
			}

			string userData = config.getString("UserData_Root", "");
			if (userData != "") {
				endPathWithSlash(userData);
			}

			//buttons
			buttonReturn.registerGraphicComponent(containerName, "buttonReturn");
			buttonReturn.init(250, 300, 150);
			buttonReturn.setText(lang.getString("Return"));

			buttonConnect.registerGraphicComponent(containerName, "buttonConnect");
			buttonConnect.init(425, 300, 150);
			buttonConnect.setText(lang.getString("Connect"));

			buttonCreateGame.registerGraphicComponent(containerName,
				"buttonCreateGame");
			buttonCreateGame.init(600, 300, 150);
			buttonCreateGame.setText(lang.getString("HostGame"));

			buttonAutoFindServers.registerGraphicComponent(containerName,
				"buttonAutoFindServers");
			buttonAutoFindServers.init(360, 250, 280);
			buttonAutoFindServers.setText(lang.getString("FindLANGames"));
			buttonAutoFindServers.setEnabled(true);

			int labelXleft = 300;
			int labelXright = 480;

			//server type label
			labelServerType.registerGraphicComponent(containerName,
				"labelServerType");
			labelServerType.init(labelXleft, 490);
			labelServerType.setText(lang.getString("ServerType"));

			//server type list box
			listBoxServerType.registerGraphicComponent(containerName,
				"listBoxServerType");
			listBoxServerType.init(labelXright, 490, 210);
			listBoxServerType.pushBackItem(lang.getString("ServerTypeNew"));
			listBoxServerType.pushBackItem(lang.getString("ServerTypePrevious"));
			listBoxServerType.pushBackItem(lang.getString("ServerTypeFound"));

			//server label
			labelServer.registerGraphicComponent(containerName, "labelServer");
			labelServer.init(labelXleft, 460);
			labelServer.setText(lang.getString("Server"));

			//server listbox
			listBoxServers.registerGraphicComponent(containerName,
				"listBoxServers");
			listBoxServers.init(labelXright, 460, 210);
			for (int i = 0; i < servers.getPropertyCount(); ++i) {
				listBoxServers.pushBackItem(servers.getKey(i));
			}

			// found servers listbox
			listBoxFoundServers.registerGraphicComponent(containerName,
				"listBoxFoundServers");
			listBoxFoundServers.init(labelXright, 460, 210);

			//server ip
			labelServerIp.registerGraphicComponent(containerName, "labelServerIp");
			labelServerIp.setEditable(true);
			labelServerIp.setMaxEditWidth(26);
			labelServerIp.setMaxEditRenderWidth(210);
			labelServerIp.init(labelXright, 460);

			// server port
			labelServerPortLabel.registerGraphicComponent(containerName,
				"labelServerPortLabel");
			labelServerPortLabel.init(labelXleft, 430);
			labelServerPortLabel.setText(lang.getString("ServerPort"));

			labelServerPort.registerGraphicComponent(containerName,
				"labelServerPort");
			labelServerPort.init(labelXright, 430);

			string host = labelServerIp.getText();
			int portNumber = config.getInt("PortServer",
				intToStr(GameConstants::serverPort).
				c_str());
			std::vector < std::string > hostPartsList;
			Tokenize(host, hostPartsList, ":");
			if (hostPartsList.size() > 1) {
				host = hostPartsList[0];
				replaceAll(hostPartsList[1], "_", "");
				portNumber = strToInt(hostPartsList[1]);
			}

			string port = " (" + intToStr(portNumber) + ")";
			labelServerPort.setText(port);

			labelStatus.registerGraphicComponent(containerName, "labelStatus");
			labelStatus.init(labelXleft, 400);
			labelStatus.setText("");

			labelInfo.registerGraphicComponent(containerName, "labelInfo");
			labelInfo.init(labelXleft, 370);
			labelInfo.setText("");

			connected = false;
			playerIndex = -1;

			//server ip
			if (connect == true) {
				string hostIP = serverIp.getString();
				if (portNumberOverride > 0) {
					hostIP += ":" + intToStr(portNumberOverride);
				}

				labelServerIp.setText(hostIP + "_");

				autoConnectToServer = true;
			} else {
				string hostIP = config.getString("ServerIp");
				if (portNumberOverride > 0) {
					hostIP += ":" + intToStr(portNumberOverride);
				}

				labelServerIp.setText(hostIP + "_");
			}

			host = labelServerIp.getText();
			portNumber =
				config.getInt("PortServer",
					intToStr(GameConstants::serverPort).c_str());
			hostPartsList.clear();
			Tokenize(host, hostPartsList, ":");
			if (hostPartsList.size() > 1) {
				host = hostPartsList[0];
				replaceAll(hostPartsList[1], "_", "");
				portNumber = strToInt(hostPartsList[1]);
			}

			port = " (" + intToStr(portNumber) + ")";
			labelServerPort.setText(port);

			GraphicComponent::applyAllCustomProperties(containerName);

			chatManager.init(&console, -1);
		}

		void MenuStateJoinGame::reloadUI() {
			Lang & lang = Lang::getInstance();
			Config & config = Config::getInstance();

			console.resetFonts();

			buttonReturn.setText(lang.getString("Return"));
			buttonConnect.setText(lang.getString("Connect"));
			buttonCreateGame.setText(lang.getString("HostGame"));
			buttonAutoFindServers.setText(lang.getString("FindLANGames"));
			labelServerType.setText(lang.getString("ServerType"));

			std::vector < string > listboxData;
			listboxData.push_back(lang.getString("ServerTypeNew"));
			listboxData.push_back(lang.getString("ServerTypePrevious"));
			listboxData.push_back(lang.getString("ServerTypeFound"));
			listBoxServerType.setItems(listboxData);

			labelServer.setText(lang.getString("Server"));

			labelServerPortLabel.setText(lang.getString("ServerPort"));

			string host = labelServerIp.getText();
			int portNumber = config.getInt("PortServer",
				intToStr(GameConstants::serverPort).
				c_str());
			std::vector < std::string > hostPartsList;
			Tokenize(host, hostPartsList, ":");
			if (hostPartsList.size() > 1) {
				host = hostPartsList[0];
				replaceAll(hostPartsList[1], "_", "");
				portNumber = strToInt(hostPartsList[1]);
			}

			string port = " (" + intToStr(portNumber) + ")";
			labelServerPort.setText(port);

			chatManager.init(&console, -1);

			GraphicComponent::reloadFontsForRegisterGraphicComponents
			(containerName);
		}

		MenuStateJoinGame::~MenuStateJoinGame() {
			abortAutoFind = true;

#ifdef DEBUG
			PRINT_DEBUG("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__,
				__LINE__);
#endif
		}

		void MenuStateJoinGame::DiscoveredServers(std::vector < string >
			serverList) {
			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s Line: %d]\n", __FILE__,
					__FUNCTION__, __LINE__);
			if (SystemFlags::VERBOSE_MODE_ENABLED)
				printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);

			if (abortAutoFind == true) {
				return;
			}
			// Testing multi-server
			//serverList.push_back("test1");
			//serverList.push_back("test2");
			//

#ifdef DEBUG
			PRINT_DEBUG("In [%s::%s Line: %d] serverList.size() = %d\n", __FILE__,
				__FUNCTION__, __LINE__, (int) serverList.size());
#endif

			autoConnectToServer = false;
			buttonAutoFindServers.setEnabled(true);
			buttonConnect.setEnabled(true);
			if (serverList.empty() == false) {
				Config & config = Config::getInstance();
				string bestIPMatch = "";
				int serverGamePort = config.getInt("PortServer",
					intToStr(GameConstants::
						serverPort).c_str());
				std::vector < std::string > localIPList =
					Socket::getLocalIPAddressList();

				if (SystemFlags::VERBOSE_MODE_ENABLED)
					printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);

				for (int idx = 0; idx < (int) serverList.size(); idx++) {

					vector < string > paramPartPortsTokens;
					Tokenize(serverList[idx], paramPartPortsTokens, ":");
					if (paramPartPortsTokens.size() >= 2
						&& paramPartPortsTokens[1].length() > 0) {
						serverGamePort = strToInt(paramPartPortsTokens[1]);
					}

					bestIPMatch = serverList[idx];

					if (SystemFlags::
						getSystemSettingType(SystemFlags::debugSystem).enabled)
						SystemFlags::OutputDebug(SystemFlags::debugSystem,
							"In [%s::%s Line: %d] bestIPMatch = [%s]\n",
							__FILE__, __FUNCTION__, __LINE__,
							bestIPMatch.c_str());
					if (localIPList.empty() == true) {
						if (SystemFlags::VERBOSE_MODE_ENABLED)
							printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__,
								__LINE__);
						break;
					}
					if (SystemFlags::
						getSystemSettingType(SystemFlags::debugSystem).enabled)
						SystemFlags::OutputDebug(SystemFlags::debugSystem,
							"In [%s::%s Line: %d] bestIPMatch = [%s] localIPList[0] = [%s]\n",
							__FILE__, __FUNCTION__, __LINE__,
							bestIPMatch.c_str(),
							localIPList[0].c_str());
					if (strncmp(localIPList[0].c_str(), serverList[idx].c_str(), 4)
						== 0) {
						if (SystemFlags::VERBOSE_MODE_ENABLED)
							printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__,
								__LINE__);
						break;
					}
				}

				if (SystemFlags::VERBOSE_MODE_ENABLED)
					printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);

				if (bestIPMatch != "") {
					bestIPMatch += ":" + intToStr(serverGamePort);
				}
				labelServerIp.setText(bestIPMatch);

				if (SystemFlags::VERBOSE_MODE_ENABLED)
					printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);

				if (serverList.size() > 1) {
					listBoxServerType.setSelectedItemIndex(MenuStateJoinGame::
						foundServersIndex);
					listBoxFoundServers.setItems(serverList);
				} else {
					autoConnectToServer = true;
				}
			}
			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s Line: %d]\n", __FILE__,
					__FUNCTION__, __LINE__);
			if (SystemFlags::VERBOSE_MODE_ENABLED)
				printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
		}

		void MenuStateJoinGame::mouseClick(int x, int y, MouseButton mouseButton) {
			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s] START\n", __FILE__,
					__FUNCTION__);

			CoreData & coreData = CoreData::getInstance();
			SoundRenderer & soundRenderer = SoundRenderer::getInstance();
			NetworkManager & networkManager = NetworkManager::getInstance();
			ClientInterface *clientInterface = networkManager.getClientInterface();

			if (clientInterface->isConnected() == false) {
				//server type
				if (listBoxServerType.mouseClick(x, y)) {
					if (!listBoxServers.getText().empty()) {
						labelServerIp.
							setText(servers.getString(listBoxServers.getText()) + "_");
					}
				}
				//server list
				else if (listBoxServerType.getSelectedItemIndex() ==
					newPrevServerIndex) {
					if (listBoxServers.mouseClick(x, y)) {
						labelServerIp.
							setText(servers.getString(listBoxServers.getText()) + "_");
					}
				} else if (listBoxServerType.getSelectedItemIndex() ==
					foundServersIndex) {
					if (listBoxFoundServers.mouseClick(x, y)) {
						labelServerIp.setText(listBoxFoundServers.getText());
					}
				}

				string host = labelServerIp.getText();
				Config & config = Config::getInstance();
				int portNumber = config.getInt("PortServer",
					intToStr(GameConstants::serverPort).
					c_str());
				std::vector < std::string > hostPartsList;
				Tokenize(host, hostPartsList, ":");
				if (hostPartsList.size() > 1) {
					host = hostPartsList[0];
					replaceAll(hostPartsList[1], "_", "");
					portNumber = strToInt(hostPartsList[1]);
				}

				string port = " (" + intToStr(portNumber) + ")";
				labelServerPort.setText(port);

			}

			//return
			if (buttonReturn.mouseClick(x, y)) {
				soundRenderer.playFx(coreData.getClickSoundA());

				clientInterface->stopServerDiscovery();

				if (clientInterface->getSocket() != NULL) {
					//if(clientInterface->isConnected() == true) {
					//    string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
					//    clientInterface->sendTextMessage(sQuitText,-1);
					//}
					clientInterface->close();
				}
				abortAutoFind = true;
				mainMenu->setState(new MenuStateNewGame(program, mainMenu));
				return;
			}

			//connect
			else if (buttonConnect.mouseClick(x, y)
				&& buttonConnect.getEnabled() == true) {
				ClientInterface *clientInterface =
					networkManager.getClientInterface();

				soundRenderer.playFx(coreData.getClickSoundB());
				labelInfo.setText("");

				if (clientInterface->isConnected()) {
					clientInterface->reset();
				} else {
					if (connectToServer() == true) {
						return;
					}
				}
			} else if (buttonCreateGame.mouseClick(x, y)) {
				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);
				soundRenderer.playFx(coreData.getClickSoundB());

				clientInterface->stopServerDiscovery();
				if (clientInterface->getSocket() != NULL) {
					clientInterface->close();
				}
				abortAutoFind = true;
				mainMenu->setState(new
					MenuStateCustomGame(program, mainMenu, true,
						pLanGame));
				return;
			}

			else if (buttonAutoFindServers.mouseClick(x, y)
				&& buttonAutoFindServers.getEnabled() == true) {
				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);

				ClientInterface *clientInterface =
					networkManager.getClientInterface();
				soundRenderer.playFx(coreData.getClickSoundB());

				// Triggers a thread which calls back into MenuStateJoinGame::DiscoveredServers
				// with the results
				if (clientInterface->isConnected() == false) {
					if (SystemFlags::
						getSystemSettingType(SystemFlags::debugSystem).enabled)
						SystemFlags::OutputDebug(SystemFlags::debugSystem,
							"In [%s::%s Line: %d]\n", __FILE__,
							__FUNCTION__, __LINE__);

					buttonAutoFindServers.setEnabled(false);
					buttonConnect.setEnabled(false);
					clientInterface->discoverServers(this);
				}
				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);
			}

			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s] END\n", __FILE__,
					__FUNCTION__);
		}

		void MenuStateJoinGame::mouseMove(int x, int y, const MouseState * ms) {
			buttonReturn.mouseMove(x, y);
			buttonConnect.mouseMove(x, y);
			buttonAutoFindServers.mouseMove(x, y);
			buttonCreateGame.mouseMove(x, y);
			listBoxServerType.mouseMove(x, y);

			//hide-show options depending on the selection
			if (listBoxServers.getSelectedItemIndex() == newServerIndex) {
				labelServerIp.mouseMove(x, y);
			} else if (listBoxServers.getSelectedItemIndex() == newPrevServerIndex) {
				listBoxServers.mouseMove(x, y);
			} else {
				listBoxFoundServers.mouseMove(x, y);
			}
		}

		void MenuStateJoinGame::render() {
			Renderer & renderer = Renderer::getInstance();

			renderer.renderButton(&buttonReturn);
			renderer.renderLabel(&labelServer);
			renderer.renderLabel(&labelServerType);
			renderer.renderLabel(&labelStatus);
			renderer.renderLabel(&labelInfo);
			renderer.renderLabel(&labelServerPort);
			renderer.renderLabel(&labelServerPortLabel);
			renderer.renderButton(&buttonConnect);
			renderer.renderButton(&buttonCreateGame);

			renderer.renderButton(&buttonAutoFindServers);
			renderer.renderListBox(&listBoxServerType);

			if (listBoxServerType.getSelectedItemIndex() == newServerIndex) {
				renderer.renderLabel(&labelServerIp);
			} else if (listBoxServerType.getSelectedItemIndex() ==
				newPrevServerIndex) {
				renderer.renderListBox(&listBoxServers);
			} else {
				renderer.renderListBox(&listBoxFoundServers);
			}

			renderer.renderChatManager(&chatManager);
			renderer.renderConsole(&console);

			if (program != NULL)
				program->renderProgramMsgBox();
		}

		void MenuStateJoinGame::update() {
			ClientInterface *clientInterface =
				NetworkManager::getInstance().getClientInterface();
			Lang & lang = Lang::getInstance();

			//update status label
			if (clientInterface->isConnected()) {
				buttonConnect.setText(lang.getString("Disconnect"));

				if (clientInterface->getAllowDownloadDataSynch() == false) {
					string label = lang.getString("ConnectedToServer");

					if (!clientInterface->getServerName().empty()) {
						label = label + " " + clientInterface->getServerName();
					}

					if (clientInterface->getAllowGameDataSynchCheck() == true &&
						clientInterface->getNetworkGameDataSynchCheckOk() == false) {
						label = label + " - warning synch mismatch for:";
						if (clientInterface->getNetworkGameDataSynchCheckOkMap() ==
							false) {
							label = label + " map";
						}
						if (clientInterface->getNetworkGameDataSynchCheckOkTile() ==
							false) {
							label = label + " tile";
						}
						if (clientInterface->getNetworkGameDataSynchCheckOkTech() ==
							false) {
							label = label + " techtree";
						}
					} else if (clientInterface->getAllowGameDataSynchCheck() == true) {
						label += " - data synch is ok";
					}
					labelStatus.setText(label);
				} else {
					string label = lang.getString("ConnectedToServer");

					if (!clientInterface->getServerName().empty()) {
						label = label + " " + clientInterface->getServerName();
					}

					if (clientInterface->getAllowGameDataSynchCheck() == true &&
						clientInterface->getNetworkGameDataSynchCheckOk() == false) {
						label = label + " - waiting to synch:";
						if (clientInterface->getNetworkGameDataSynchCheckOkMap() ==
							false) {
							label = label + " map";
						}
						if (clientInterface->getNetworkGameDataSynchCheckOkTile() ==
							false) {
							label = label + " tile";
						}
						if (clientInterface->getNetworkGameDataSynchCheckOkTech() ==
							false) {
							label = label + " techtree";
						}
					} else if (clientInterface->getAllowGameDataSynchCheck() == true) {
						label += " - data synch is ok";
					}

					labelStatus.setText(label);
				}
			} else {
				buttonConnect.setText(lang.getString("Connect"));
				string connectedStatus = lang.getString("NotConnected");
				if (buttonAutoFindServers.getEnabled() == false) {
					connectedStatus += " - searching for servers, please wait...";
				}
				labelStatus.setText(connectedStatus);
				labelInfo.setText("");
			}

			//process network messages
			if (clientInterface->isConnected()) {
				//update lobby
				clientInterface->updateLobby();

				clientInterface =
					NetworkManager::getInstance().getClientInterface();
				if (clientInterface != NULL && clientInterface->isConnected()) {
					//call the chat manager
					chatManager.updateNetwork();

					//console
					console.update();

					//intro
					if (clientInterface->getIntroDone()) {
						labelInfo.setText(lang.getString("WaitingHost"));

						string host = labelServerIp.getText();
						std::vector < std::string > hostPartsList;
						Tokenize(host, hostPartsList, ":");
						if (hostPartsList.size() > 1) {
							host = hostPartsList[0];
							replaceAll(hostPartsList[1], "_", "");
						}
						string saveHost = Ip(host).getString();
						if (hostPartsList.size() > 1) {
							saveHost += ":" + hostPartsList[1];
						}

						servers.setString(clientInterface->getServerName(), saveHost);
					}

					//launch
					if (clientInterface->getLaunchGame()) {
						if (SystemFlags::
							getSystemSettingType(SystemFlags::debugSystem).enabled)
							SystemFlags::OutputDebug(SystemFlags::debugSystem,
								"In [%s::%s] clientInterface->getLaunchGame() - A\n",
								__FILE__, __FUNCTION__);

						if (SystemFlags::
							getSystemSettingType(SystemFlags::debugSystem).enabled)
							SystemFlags::OutputDebug(SystemFlags::debugSystem,
								"In [%s::%s] clientInterface->getLaunchGame() - B\n",
								__FILE__, __FUNCTION__);

						abortAutoFind = true;
						clientInterface->stopServerDiscovery();
						program->setState(new
							Game(program,
								clientInterface->getGameSettings(),
								false));
						return;
					}
				}
			} else if (autoConnectToServer == true) {
				autoConnectToServer = false;
				if (connectToServer() == true) {
					return;
				}
			}

			if (clientInterface != NULL && clientInterface->getLaunchGame())
				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s] clientInterface->getLaunchGame() - D\n",
						__FILE__, __FUNCTION__);
		}

		bool MenuStateJoinGame::textInput(std::string text) {
			if (chatManager.getEditEnabled() == true) {
				return chatManager.textInput(text);
			}
			return false;
		}

		void MenuStateJoinGame::keyDown(SDL_KeyboardEvent key) {
			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s Line: %d] key = [%c][%d]\n",
					__FILE__, __FUNCTION__, __LINE__,
					key.keysym.sym, key.keysym.sym);

			ClientInterface *clientInterface =
				NetworkManager::getInstance().getClientInterface();
			if (clientInterface->isConnected() == false) {
				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);

				Config & configKeys =
					Config::getInstance(std::pair < ConfigType,
						ConfigType >(cfgMainKeys, cfgUserKeys));

				string text = labelServerIp.getText();
				if (isKeyPressed(SDLK_BACKSPACE, key) == true && text.length() > 0) {
					if (SystemFlags::
						getSystemSettingType(SystemFlags::debugSystem).enabled)
						SystemFlags::OutputDebug(SystemFlags::debugSystem,
							"In [%s::%s Line: %d]\n", __FILE__,
							__FUNCTION__, __LINE__);
					size_t found = text.find_last_of("_");
					if (found == string::npos) {
						text.erase(text.end() - 1);
					} else {
						if (text.size() > 1) {
							text.erase(text.end() - 2);
						}
					}

					labelServerIp.setText(text);
				}
				//else if(key == configKeys.getCharKey("SaveGUILayout")) {
				else if (isKeyPressed(configKeys.getSDLKey("SaveGUILayout"), key) ==
					true) {
					bool saved =
						GraphicComponent::saveAllCustomProperties(containerName);
					Lang & lang = Lang::getInstance();
					console.addLine(lang.getString("GUILayoutSaved") + " [" +
						(saved ? lang.
							getString("Yes") : lang.getString("No")) + "]");
				}
			} else {
				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line: %d]\n", __FILE__,
						__FUNCTION__, __LINE__);

				//send key to the chat manager
				chatManager.keyDown(key);

				if (chatManager.getEditEnabled() == false) {
					Config & configKeys =
						Config::getInstance(std::pair < ConfigType,
							ConfigType >(cfgMainKeys, cfgUserKeys));
					//if(key == configKeys.getCharKey("SaveGUILayout")) {
					if (isKeyPressed(configKeys.getSDLKey("SaveGUILayout"), key) ==
						true) {
						bool saved =
							GraphicComponent::saveAllCustomProperties(containerName);
						Lang & lang = Lang::getInstance();
						console.addLine(lang.getString("GUILayoutSaved") + " [" +
							(saved ? lang.
								getString("Yes") : lang.getString("No")) +
							"]");
					}
				}
			}
		}

		void MenuStateJoinGame::keyPress(SDL_KeyboardEvent c) {
			ClientInterface *clientInterface =
				NetworkManager::getInstance().getClientInterface();

			if (clientInterface->isConnected() == false) {
				int maxTextSize = 22;

				//Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

				SDL_Keycode key = extractKeyPressed(c);

				//if(c>='0' && c<='9') {
				if ((key >= SDLK_0 && key <= SDLK_9) ||
					(key >= SDLK_KP_0 && key <= SDLK_KP_9)) {
					if ((int) labelServerIp.getText().size() < maxTextSize) {
						string text = labelServerIp.getText();
						//text.insert(text.end()-1, key);
						char szCharText[26] = "";
						snprintf(szCharText, 26, "%c", key);
						char *utfStr = ConvertToUTF8(&szCharText[0]);
						if (text.size() > 0) {
							text.insert(text.end() - 1, utfStr[0]);
						} else {
							text = utfStr[0];
						}

						delete[]utfStr;

						labelServerIp.setText(text);
					}
				}
				//else if (c=='.') {
				else if (key == SDLK_PERIOD) {
					if ((int) labelServerIp.getText().size() < maxTextSize) {
						string text = labelServerIp.getText();
						if (text.size() > 0) {
							text.insert(text.end() - 1, '.');
						} else {
							text = ".";
						}

						labelServerIp.setText(text);
					}
				}
				//else if (c==':') {
				else if (key == SDLK_COLON) {
					if ((int) labelServerIp.getText().size() < maxTextSize) {
						string text = labelServerIp.getText();
						if (text.size() > 0) {
							text.insert(text.end() - 1, ':');
						} else {
							text = ":";
						}

						labelServerIp.setText(text);
					}
				}
			} else {
				chatManager.keyPress(c);
			}
		}

		bool MenuStateJoinGame::connectToServer() {
			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s] START\n", __FILE__,
					__FUNCTION__);

			Config & config = Config::getInstance();
			string host = labelServerIp.getText();
			int port = config.getInt("PortServer",
				intToStr(GameConstants::serverPort).
				c_str());
			std::vector < std::string > hostPartsList;
			Tokenize(host, hostPartsList, ":");
			if (hostPartsList.size() > 1) {
				host = hostPartsList[0];
				replaceAll(hostPartsList[1], "_", "");
				port = strToInt(hostPartsList[1]);
			}
			Ip serverIp(host);

			ClientInterface *clientInterface =
				NetworkManager::getInstance().getClientInterface();
			clientInterface->connect(serverIp, port);

			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s] server - [%s]\n", __FILE__,
					__FUNCTION__,
					serverIp.getString().c_str());

			labelServerIp.setText(serverIp.getString() + '_');
			labelInfo.setText("");

			//save server ip
			if (config.getString("ServerIp") != serverIp.getString()) {
				config.setString("ServerIp", serverIp.getString());
				config.save();
			}

			for (time_t elapsedWait = time(NULL);
				clientInterface->getIntroDone() == false &&
				clientInterface->isConnected() &&
				difftime(time(NULL), elapsedWait) <= 10;) {
				if (clientInterface->isConnected()) {
					//update lobby
					clientInterface->updateLobby();
					sleep(0);
					//this->render();
				}
			}
			if (clientInterface->isConnected() == true &&
				clientInterface->getIntroDone() == true) {

				string saveHost = Ip(host).getString();
				if (hostPartsList.size() > 1) {
					saveHost += ":" + hostPartsList[1];
				}
				servers.setString(clientInterface->getServerName(), saveHost);

				if (SystemFlags::
					getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,
						"In [%s::%s Line %d] Using FTP port #: %d\n",
						__FILE__, __FUNCTION__, __LINE__,
						clientInterface->getServerFTPPort());
				abortAutoFind = true;
				clientInterface->stopServerDiscovery();
				mainMenu->setState(new MenuStateConnectedGame(program, mainMenu));
				return true;
			}
			if (SystemFlags::
				getSystemSettingType(SystemFlags::debugSystem).enabled)
				SystemFlags::OutputDebug(SystemFlags::debugSystem,
					"In [%s::%s] END\n", __FILE__,
					__FUNCTION__);
			return false;
		}

	}
}                               //end namespace
