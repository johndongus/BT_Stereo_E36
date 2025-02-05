// BT.h
#ifndef BT_H
#define BT_H

#include <gio/gio.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>

class BluetoothAgent;

struct MediaInfo {
    std::string title;
    std::string artist;
    int duration;    
    int progress;   
    std::string status;
};
struct CallInfo {
    std::string state;      
    std::string number;    
    std::string contact;    // Contact name (if available)
};

struct PairingRequest {
    std::string device_path;
    std::string passkey;
    bool requires_confirmation = false;
    bool active = false;

    std::function<void(bool)> respond; // Callback function to accept/reject
};


class BluetoothMedia {
public:
    BluetoothMedia();
    ~BluetoothMedia();
 bool auto_pairing_enabled = true;
    void setup_bluetooth();
    void start_media_monitor();
    MediaInfo get_media_info() ;
    void trust_connected_device(); 
    std::atomic<bool> is_disconnected{false}; 
    mutable std::mutex media_mutex; 
    int get_song_duration() const;     
    int get_song_progress() ;      
    void play_pause();        
    void next_track();           
    void previous_track();         
    void on_call_added(const gchar* manager_path, GVariant* parameters);
    void on_call_removed(const gchar* manager_path, GVariant* parameters);
    bool g_call_in_progress = false;           
    std::string g_current_caller_number;       
    std::string g_current_call_path;           
    void handle_interfaces_removed(const gchar* object_path, GVariant* parameters);
    std::chrono::steady_clock::time_point last_retry_time;
    const int retry_interval_ms = 1000; 
    int last_song_progress;
    MediaInfo last_media_info;
bool connect_to_device(const std::string& device_path);
bool remove_trusted_device(const std::string& device_path);
std::vector<std::string> get_trusted_devices();

    std::string get_playback_status(); // Retrieves current playback status
    std::string last_incoming_call_path;
    std::string connected_device_name;
    void poll_ofono_calls();
    void monitor_calls();   // Method to monitor incoming calls
    void handle_interfaces_added(GVariant* parameters);
    bool supports_voice_call_manager(GVariant* modem_properties);
    void handle_call_signal(const gchar* object_path, GVariant* parameters);
    bool check_and_set_media_player_path(); // Moved to public

    static void on_properties_changed(GDBusConnection* conn, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data);

        bool is_media_player_connected() const {
        return !media_player_path.empty() && !is_disconnected.load();
    }
    bool has_active_pairing_request() {
        std::lock_guard<std::mutex> lock(pairing_mutex);
         std::cout << "[DEBUG] Pairing flag is active for device: " << this->current_pairing_request.device_path << std::endl;
        return current_pairing_request.active;
    }
        PairingRequest current_pairing_request;
    std::mutex pairing_mutex;  // Ensure thread safety
    void accept_pairing();
    void reject_pairing();

private:
    GDBusConnection* connection;
    GMainLoop* main_loop = nullptr;
    std::string media_player_path;



    std::string find_adapter_path();
    std::string find_media_player_path();
};

class BluetoothAgent {
public:
    BluetoothAgent(GDBusConnection* conn, const std::string& object_path);
    ~BluetoothAgent();

    void register_agent();
    void unregister_agent();

private:
    static void handle_method_call(
        GDBusConnection* connection,
        const gchar* sender,
        const gchar* object_path,
        const gchar* interface_name,
        const gchar* method_name,
        GVariant* parameters,
        GDBusMethodInvocation* invocation,
        gpointer user_data
    );

    static const GDBusInterfaceVTable vtable;

    GDBusConnection* connection;
    std::string agent_path;
    guint registration_id;
    GDBusNodeInfo* introspection_data;
};

#endif // BT_H
