/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <app_options/AppOptions.h>
#include <devices/device-factory/DeviceFactory.h>
#include <lib/support/CodeUtils.h>
#include <platform/CHIPDeviceConfig.h>

#include <cstdlib>
#include <cstring>

using namespace chip;

DeviceTypeParser AppOptions::sParser;
AppOptions::AppConfig AppOptions::mConfig;
bool AppOptions::mShowHelp = false;

const AppOptions::AppConfig & AppOptions::GetConfig()
{
    if (mConfig.deviceTypeEntries.empty())
    {
        mConfig.deviceTypeEntries.push_back({
            .type     = chip::app::DeviceFactory::GetInstance().GetDefaultDevice(),
            .endpoint = 1,
            .parentId = chip::kInvalidEndpointId,
        });
    }
    return mConfig;
}

lyra::cli & AppOptions::GetCli()
{
    static lyra::cli sCli;

    static bool sInitialized = false;
    if (!sInitialized)
    {
        sInitialized = true;

        // --device <type> or <type:endpoint> or <type:endpoint,parent=parentId>
        // Can be specified multiple times.
        sCli |= lyra::opt(
                    [](const std::string & value) {
                        if (sParser.ParseSingleDeviceString(value.c_str()) != CHIP_NO_ERROR)
                        {
                            ChipLogError(Support, "Invalid device specification: %s", value.c_str());
                        }
                        else
                        {
                            mConfig.deviceTypeEntries = sParser.GetDeviceTypeEntries();
                        }
                    },
                    "device")
                    ["--device"]("Select the device to start up. Format: 'type' or 'type:endpoint' or "
                                 "'type:endpoint,parent=parentId'. Can be specified multiple times.");

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
        sCli |= lyra::opt(mConfig.bleController, "number")["--ble-controller"]("Select the BLE controller to use (default: 0)");
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
        sCli |= lyra::opt([&](bool) {
                    mConfig.enableWiFi = true;
                    ChipLogProgress(AppServer, "WiFi usage enabled");
                })["--wifi"]("Enable wifi support for commissioning");
#endif

        sCli |= lyra::opt(mConfig.kvsPath, "path")["--KVS"]("Key-Value Store path");

        sCli |= lyra::opt(
                    [](const std::string & value) {
                        unsigned long val = std::strtoul(value.c_str(), nullptr, 0);
                        if (val > 0xFFF)
                        {
                            ChipLogError(Support, "Invalid discriminator: %s", value.c_str());
                            return;
                        }
                        mConfig.discriminator = static_cast<uint16_t>(val);
                    },
                    "discriminator")["--discriminator"]("Setup discriminator (12-bit value, 0-4095)");

        sCli |= lyra::opt(
                    [](const std::string & value) {
                        mConfig.vendorId = static_cast<uint16_t>(std::strtoul(value.c_str(), nullptr, 0));
                    },
                    "vendor-id")["--vendor-id"]("Vendor ID");

        sCli |= lyra::opt(
                    [](const std::string & value) {
                        mConfig.productId = static_cast<uint16_t>(std::strtoul(value.c_str(), nullptr, 0));
                    },
                    "product-id")["--product-id"]("Product ID");

        sCli |= lyra::opt(
                    [](const std::string & value) {
                        unsigned long val = std::strtoul(value.c_str(), nullptr, 0);
                        if (val > 0xFFFF)
                        {
                            ChipLogError(Support, "Invalid port: %s", value.c_str());
                            return;
                        }
                        mConfig.port = static_cast<uint16_t>(val);
                        ChipLogProgress(AppServer, "Port option set to %u", static_cast<uint16_t>(val));
                    },
                    "port")["--port"]("Operational port");

        sCli |= lyra::opt(
                    [](const std::string & value) {
                        mConfig.interfaceId = static_cast<uint32_t>(std::strtoul(value.c_str(), nullptr, 0));
                    },
                    "interface-id")["--interface-id"]("Network interface ID");

        sCli |= lyra::opt([&](bool) {
                    mConfig.enableGroupcast = true;
                    ChipLogProgress(AppServer, "Groupcast usage enabled");
                })["--groupcast"]("Enable groupcast support");

        sCli |= lyra::help(mShowHelp);
    }

    return sCli;
}

bool AppOptions::ShowHelp()
{
    return mShowHelp;
}
