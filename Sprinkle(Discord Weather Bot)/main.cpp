#include <dpp/dpp.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <map>
#include <chrono>
#include <thread>

using json = nlohmann::json;

struct CityRole {
    std::string city;
    dpp::snowflake role_id;
};

std::map<dpp::snowflake, CityRole> city_roles;
const std::string WEATHER_API_KEY = "your_weather_api_key";
const dpp::snowflake NOTIFICATION_CHANNEL_ID = 123456789;

json fetch_weather_data(const std::string& city) {
    try {
        auto response = cpr::Get(cpr::Url{"http://api.openweathermap.org/data/2.5/forecast"},
                               cpr::Parameters{{"q", city},
                                             {"appid", WEATHER_API_KEY},
                                             {"units", "metric"}});
        
        if (response.status_code == 200) {
            return json::parse(response.text);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error fetching weather: " << e.what() << std::endl;
    }
    return json();
}

bool is_early_morning() {
    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    tm local_tm = *localtime(&tt);
    return local_tm.tm_hour == 4 && local_tm.tm_min < 5;
}

std::string format_temperature(double temp) {
    return std::to_string(static_cast<int>(std::round(temp))) + "°C";
}

int main() {
    dpp::cluster bot("your_bot_token");

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(
                dpp::slashcommand("setcity", "Set city for a role", bot.me.id)
                .add_option(dpp::command_option(dpp::co_role, "role", "The role to set city for", true))
                .add_option(dpp::command_option(dpp::co_string, "city", "The city name", true))
            );
            
            bot.global_command_create(
                dpp::slashcommand("forecast", "Get weather forecast for a city", bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "city", "The city name", true))
            );
        }
    });

    bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "setcity") {
            dpp::role* role = std::get<dpp::role*>(event.get_parameter("role"));
            std::string city = std::get<std::string>(event.get_parameter("city"));
            
            city_roles[role->id] = {city, role->id};
            event.reply("Set " + city + " as the city for role " + role->name);
        }
        else if (event.command.get_command_name() == "forecast") {
            std::string city = std::get<std::string>(event.get_parameter("city"));
            json weather_data = fetch_weather_data(city);
            
            if (!weather_data.empty()) {
                std::string message = "**Weather Forecast for " + city + "**\n";
                for (const auto& item : weather_data["list"]) {
                    std::string time = item["dt_txt"];
                    double temp = item["main"]["temp"];
                    double rain_prob = item["pop"].get<double>() * 100;
                    
                    message += time + ": " + format_temperature(temp) + ", Rain Probability: " + 
                              std::to_string(static_cast<int>(rain_prob)) + "%\n";
                }
                event.reply(message);
            } else {
                event.reply("Could not fetch weather data for " + city);
            }
        }
    });

    bot.start_timer([&bot](const dpp::timer& timer) {
        bool is_morning = is_early_morning();
        std::string morning_message;
        
        if (is_morning) {
            morning_message = "☀️ Good morning everyone!\nHere's today's weather forecast:\n\n";
        }

        for (const auto& [role_id, city_role] : city_roles) {
            json weather_data = fetch_weather_data(city_role.city);
            
            if (!weather_data.empty()) {
                // Regular hourly check for rain
                double rain_prob = weather_data["list"][0]["pop"].get<double>() * 100;
                
                if (rain_prob > 40) {
                    std::string alert = "🌧️ <@&" + std::to_string(role_id) + "> High chance of rain (" +
                                      std::to_string(static_cast<int>(rain_prob)) + "%) in " + city_role.city +
                                      "\nDon't forget your umbrella!";
                    
                    dpp::message m(NOTIFICATION_CHANNEL_ID, alert);
                    bot.message_create(m);
                }
                
                // Add to morning forecast if it's morning
                if (is_morning) {
                    morning_message += "**" + city_role.city + "**\n";
                    
                    // Get today's date for comparison
                    auto now = std::chrono::system_clock::now();
                    auto today = std::chrono::floor<std::chrono::days>(now);
                    
                    // Add forecast for next 24 hours
                    for (const auto& item : weather_data["list"]) {
                        std::string time = item["dt_txt"];
                        double temp = item["main"]["temp"];
                        double rain_prob = item["pop"].get<double>() * 100;
                        std::string conditions = item["weather"][0]["main"];
                        
                        morning_message += time + ": " + format_temperature(temp) + ", " +
                                         conditions + ", " +
                                         std::to_string(static_cast<int>(rain_prob)) + "% chance of rain\n";
                    }
                    morning_message += "\n";
                }
            }
        }

        // Send combined morning message if it's morning
        if (is_morning && !morning_message.empty()) {
            dpp::message m(NOTIFICATION_CHANNEL_ID, morning_message);
            bot.message_create(m);
        }
    }, 3600); // Check every hour

    bot.start(dpp::st_wait);
    return 0;
}
