#include "BT.h"
#include <stdexcept>
#include <thread>
#include <algorithm>
#define GVARIANT_TO_CSTR(var) (var ? g_variant_print(var, TRUE) : "(null)")

#include <unordered_map>
std::unordered_map<std::string, std::string> call_states; // call_path -> "incoming"/"active"/etc.


//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣴⣿⠽⠭⣥⣤⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡴⠞⠉⠁⠀⠀⠀⠀⠉⠉⠛⠶⣤⣀⠀⠀⢀⣤⠴⠞⠛⠉⠉⠉⠛⠶⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡾⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠳⣏⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢻⣆⠀⠀⠀⠀⠀⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⠏⠀⠀⠀⠀⠀⠀⢀⣠⠤⠤⠤⠤⢤⣄⡀⠀⠀⠹⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⡄⠀⠀⠀⠀⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⢀⡾⠁⠀⠀⠀⠀⠀⠐⠈⠁⠀⠀⠀⠀⠀⠀⠀⠉⠛⠶⢤⣽⡦⠐⠒⠒⠂⠀⠀⠀⠀⠐⠒⠀⢿⣦⣀⡀⠀⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⢀⡞⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣠⡤⠤⠤⠤⠤⠠⠌⢻⣆⡀⠀⠀⠀⣀⣀⣀⡀⠤⠤⠄⠠⢉⣙⡿⣆⡀⠀
//⠀⠀⠀⠀⣀⣴⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣤⢶⣛⣩⣶⣶⡾⢯⠿⠷⣖⣦⣤⣍⣿⣴⠖⣋⠭⣷⣶⣶⡶⠒⠒⣶⣒⣠⣀⣙⣿⣆
//⠀⠀⢀⠞⠋⠀⡇⠀⠀⠀⠀⠀⠀⢀⣠⡶⣻⡯⣲⡿⠟⢋⣵⣛⣾⣿⣷⡄⠀⠈⠉⠙⠛⢻⣯⠤⠚⠋⢉⣴⣻⣿⣿⣷⣼⠁⠉⠛⠺⣿
//⠀⣠⠎⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⣟⣫⣿⠟⠉⠀⠀⣾⣿⣻⣿⣤⣿⣿⠀⠀⠀⠀⠀⠀⣿⠀⠀⠀⣿⣿⣻⣿⣼⣿⣿⠇⠀⠀⠀⢙
//⢠⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠛⡶⣄⠀⠀⢻⣿⣿⣿⣿⣿⡏⠀⠀⠀⣀⣤⣾⣁⠀⠀⠀⠸⢿⣿⣿⣿⡿⠋⠀⣀⣠⣶⣿
//⠟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠺⢿⣶⣶⣮⣭⣭⣭⣭⡴⢶⣶⣾⠿⠟⠋⠉⠉⠙⠒⠒⠊⠉⠈⠉⠚⠉⠉⢉⣷⡾⠯
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠉⠀⠀⠀⢈⣽⠟⠁⠀⠀⠀⠀⣄⡀⠀⠀⠀⠀⠀⠀⢀⣴⡾⠟⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣤⡴⠞⠋⠁⠀⠀⠀⠀⠀⠀⠈⠙⢷⡀⠉⠉⠉⠀⠙⢿⣵⡄⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢷⡀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣀⣀⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣧⠀⠀                               
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣴⠟⠋⠉⠀⠀⠉⠛⠛⠛⠛⠷⠶⠶⠶⠶⠤⢤⣤⣤⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣠⡤⢿⣆⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡶⠋⠀⠀⠀⠸⠿⠛⠛⠛⠓⠒⠲⠶⢤⣤⣄⣀⠀⠀⠀⠈⠙⠛⠛⠛⠛⠒⠶⠶⠶⣶⠖⠛⠛⠁⢠⣸⡟⠀
//⠀⠀⠀⠀⠀⠀⢰⣆⠀⢸⣧⣤⣤⣤⣤⣤⣤⣤⣤⣤⣀⠀⠀⠀⠀⠀⠉⠉⠛⠛⠓⠒⠲⠦⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣤⣾⠋⠀⠀
//⡀⠀⠀⠀⠀⠀⠀⠙⢷⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠉⠛⠲⠶⣶⣤⣄⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡾⠃⠀⠀⠀                 i do not like bluetooth
//⣿⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠉⠉⠉⠉⠉⠉⠉⠉⠛⠛⣳⣶⡶⠟⠉⠀⠀⠀⠀⠀
//⠛⢷⣿⣷⠤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣴⠟⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀
//⠀⠀⠈⠙⠻⢷⣬⣗⣒⣂⡀⠠⠀⠀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣀⣀⣤⡴⠾⠋⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠉⠉⠛⠛⠿⠶⠶⠶⠶⣤⣤⣭⣭⣍⣉⣉⣀⣀⣀⣀⣼⣯⡽⠷⠿⠛⠙⠿⣦⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠉⠉⠉⠉⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢷⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡀⠀⠀⠀⠀⠈⠻⣦⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀

BluetoothMedia::BluetoothMedia() {
    GError* error = nullptr;
    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (!connection) {
        std::cerr << "Failed to connect to system bus: " << error->message << std::endl;
        g_error_free(error);
        throw std::runtime_error("Failed to connect to D-Bus.");
    }


   // monitor_calls();
}

BluetoothMedia::~BluetoothMedia() {
    if (connection) {
        g_object_unref(connection);
        connection = nullptr;
    }
    if (main_loop) {
        g_main_loop_quit(main_loop);
        g_main_loop_unref(main_loop);
        main_loop = nullptr;
    }
}
void BluetoothMedia::poll_ofono_calls()
{  
    if (!connection) {
        std::cerr << "No DBus connection.\n";
        return;
    }

    // 1) Get all modems from Ofono
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.ofono",
        "/",
        "org.ofono.Manager",
        "GetModems",
        nullptr,
        nullptr, // no format string
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "poll_ofono_calls: GetModems call failed: "
                  << (error ? error->message : "(unknown)") << std::endl;
        if (error) g_error_free(error);
        return;
    }


    if (!g_variant_is_of_type(result, G_VARIANT_TYPE_TUPLE)
        || g_variant_n_children(result) < 1)
    {
        std::cerr << "poll_ofono_calls: unexpected GetModems reply.\n";
        g_variant_unref(result);
        return;
    }

    GVariant* modems_array = g_variant_get_child_value(result, 0);
    g_variant_unref(result);

    if (!modems_array || !g_variant_is_of_type(modems_array, G_VARIANT_TYPE_ARRAY)) {
        std::cerr << "poll_ofono_calls: not an array.\n";
        if (modems_array) g_variant_unref(modems_array);
        return;
    }


    GVariantIter array_iter;
    g_variant_iter_init(&array_iter, modems_array);

    GVariant* modem_item = nullptr;
    while ((modem_item = g_variant_iter_next_value(&array_iter))) {

        if (!g_variant_is_of_type(modem_item, G_VARIANT_TYPE_TUPLE)
            || g_variant_n_children(modem_item) < 2)
        {
            g_variant_unref(modem_item);
            continue;
        }

        GVariant* path_var = g_variant_get_child_value(modem_item, 0);
        const char* modem_path = g_variant_get_string(path_var, nullptr);
        g_variant_unref(path_var);

     
        GVariant* props_var = g_variant_get_child_value(modem_item, 1);
        g_variant_unref(modem_item);

        if (!props_var || !g_variant_is_of_type(props_var, G_VARIANT_TYPE_DICTIONARY)) {
            if (props_var) g_variant_unref(props_var);
            continue;
        }

        bool has_vcm = false; // whether it has "org.ofono.VoiceCallManager"
        {
       
            GVariantIter dict_iter;
            g_variant_iter_init(&dict_iter, props_var);

            GVariant* one_entry = nullptr;
            while ((one_entry = g_variant_iter_next_value(&dict_iter))) {
             
                if (g_variant_is_of_type(one_entry, G_VARIANT_TYPE_DICT_ENTRY)
                    && g_variant_n_children(one_entry) == 2)
                {
                    GVariant* key_var = g_variant_get_child_value(one_entry, 0);
                    const char* key_str = g_variant_get_string(key_var, nullptr);
                    g_variant_unref(key_var);

                    GVariant* val_var = g_variant_get_child_value(one_entry, 1);

                   
                    if (std::string(key_str) == "Interfaces"
                        && g_variant_is_of_type(val_var, G_VARIANT_TYPE_STRING_ARRAY))
                    {
                        GVariantIter str_iter;
                        g_variant_iter_init(&str_iter, val_var);

                        const gchar* an_iface = nullptr;
                        while (g_variant_iter_next(&str_iter, "&s", &an_iface)) {
                            if (std::string(an_iface) == "org.ofono.VoiceCallManager") {
                                has_vcm = true;
                                break;
                            }
                        }
                    }
                    g_variant_unref(val_var);
                }
                g_variant_unref(one_entry);

                if (has_vcm) break;
            }
        }

        g_variant_unref(props_var);

        if (!has_vcm) {
          
            continue;
        }

        GVariant* callsResult = g_dbus_connection_call_sync(
            connection,
            "org.ofono",
            modem_path,
            "org.ofono.VoiceCallManager",
            "GetCalls",
            nullptr,
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            &error
        );

        if (!callsResult) {
            std::cerr << "poll_ofono_calls: GetCalls failed on " << modem_path
                      << ": " << (error ? error->message : "(unknown)") << "\n";
            if (error) g_error_free(error);
            continue;
        }

     
        if (!g_variant_is_of_type(callsResult, G_VARIANT_TYPE_TUPLE)
            || g_variant_n_children(callsResult) < 1)
        {
            std::cerr << "poll_ofono_calls: unexpected calls reply.\n";
            g_variant_unref(callsResult);
            continue;
        }

        GVariant* calls_array = g_variant_get_child_value(callsResult, 0);
        g_variant_unref(callsResult);

        if (!calls_array || !g_variant_is_of_type(calls_array, G_VARIANT_TYPE_ARRAY)) {
            if (calls_array) g_variant_unref(calls_array);
            continue;
        }

      
        GVariantIter calls_iter;
        g_variant_iter_init(&calls_iter, calls_array);

        GVariant* one_call = nullptr;
        while ((one_call = g_variant_iter_next_value(&calls_iter))) {
            if (!g_variant_is_of_type(one_call, G_VARIANT_TYPE_TUPLE)
                || g_variant_n_children(one_call) < 2)
            {
                g_variant_unref(one_call);
                continue;
            }

          
            GVariant* call_path_var = g_variant_get_child_value(one_call, 0);
            const char* call_path = g_variant_get_string(call_path_var, nullptr);
            g_variant_unref(call_path_var);

       
            GVariant* call_dict = g_variant_get_child_value(one_call, 1);
            g_variant_unref(one_call);

            if (!call_dict || !g_variant_is_of_type(call_dict, G_VARIANT_TYPE_DICTIONARY)) {
                if (call_dict) g_variant_unref(call_dict);
                continue;
            }

            bool is_incoming = false;
            std::string call_state;
            std::string caller_num;

            GVariantIter cdict_iter;
            g_variant_iter_init(&cdict_iter, call_dict);

            GVariant* call_entry = nullptr;
            while ((call_entry = g_variant_iter_next_value(&cdict_iter))) {
                if (g_variant_is_of_type(call_entry, G_VARIANT_TYPE_DICT_ENTRY)
                    && g_variant_n_children(call_entry) == 2)
                {
                    GVariant* key_v = g_variant_get_child_value(call_entry, 0);
                    const char* ckey = g_variant_get_string(key_v, nullptr);
                    g_variant_unref(key_v);

                    GVariant* val_v = g_variant_get_child_value(call_entry, 1);

                    if (std::string(ckey) == "State"
                        && g_variant_is_of_type(val_v, G_VARIANT_TYPE_STRING))
                    {
                        const char* st = g_variant_get_string(val_v, nullptr);
                        if (st) call_state = st;
                        if (call_state == "incoming") {
                            is_incoming = true;
                        }
                    }

                    else if (std::string(ckey) == "LineIdentification"
                             && g_variant_is_of_type(val_v, G_VARIANT_TYPE_STRING))
                    {
                        const char* num = g_variant_get_string(val_v, nullptr);
                        if (num) caller_num = num;
                    }
                    g_variant_unref(val_v);
                }
                g_variant_unref(call_entry);
            }
            g_variant_unref(call_dict);

            if (is_incoming) {
                std::cout << ">>> Incoming call on " << call_path 
                          << " from " << (caller_num.empty() ? "Unknown" : caller_num)
                          << " [state=" << call_state << "]\n";
            }
        }
        g_variant_unref(calls_array);
    }

    g_variant_unref(modems_array);
}

void BluetoothMedia::on_call_removed(const gchar* manager_path, GVariant* parameters)
{

    std::cout << "[on_call_removed] manager_path=" << manager_path << std::endl;


    if (!parameters || !g_variant_is_of_type(parameters, G_VARIANT_TYPE_TUPLE)
        || g_variant_n_children(parameters) < 1)
    {
        std::cerr << "on_call_removed: invalid parameters.\n";
        return;
    }

    GVariant* call_path_var = g_variant_get_child_value(parameters, 0);
    if (!call_path_var || !g_variant_is_of_type(call_path_var, G_VARIANT_TYPE_OBJECT_PATH)) {
        std::cerr << "on_call_removed: not an object path.\n";
        if (call_path_var) g_variant_unref(call_path_var);
        return;
    }

    const char* removed_call_path = g_variant_get_string(call_path_var, nullptr);
    g_variant_unref(call_path_var);

    std::cout << "CallRemoved => " << (removed_call_path ? removed_call_path : "(null)") << std::endl;

    auto it = call_states.find(removed_call_path);
    if (it != call_states.end()) {
        std::cout << "Removing call path from map: " << removed_call_path << std::endl;
        call_states.erase(it);
    }

    if (g_current_call_path == removed_call_path) {
        std::cout << "This was our current call => clearing g_call_in_progress.\n";
        g_call_in_progress = false;
        g_current_call_path.clear();
        g_current_caller_number.clear();
    }
}
void BluetoothMedia::handle_interfaces_removed(const gchar* object_path, GVariant* parameters) {
    std::cout << "InterfacesRemoved signal received for object_path: " << object_path << std::endl;

    if (std::string(object_path) == media_player_path) {
        std::cerr << "MediaPlayer object was removed. Resetting media_player_path." << std::endl;
        media_player_path.clear();
    }
}
void BluetoothMedia::monitor_calls()
{
    if (!connection) {
        std::cerr << "No D-Bus connection available.\n";
        return;
    }

    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.ofono",
        "/",
        "org.ofono.Manager",
        "GetModems",
        /* no args */ nullptr,
        /* no format string to avoid mismatches */ nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,  // default timeout
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "GetModems call failed: "
                  << (error ? error->message : "unknown error") << std::endl;
        if (error) g_error_free(error);
        return;
    }

    std::cout << "Received GetModems result: "
              << g_variant_print(result, TRUE) << std::endl;

    if (!g_variant_is_of_type(result, G_VARIANT_TYPE_TUPLE)
        || g_variant_n_children(result) < 1)
    {
        std::cerr << "GetModems returned unexpected variant type.\n";
        g_variant_unref(result);
        return;
    }

    GVariant* modems_array = g_variant_get_child_value(result, 0);
    if (!modems_array || !g_variant_is_of_type(modems_array, G_VARIANT_TYPE_ARRAY)) {
        std::cerr << "Child #0 is not an array.\n";
        if (modems_array) g_variant_unref(modems_array);
        g_variant_unref(result);
        return;
    }

    std::cout << "Modems array: " << g_variant_print(modems_array, TRUE) << std::endl;

    GVariantIter array_iter;
    g_variant_iter_init(&array_iter, modems_array);

    GVariant* one_modem_tuple = nullptr;
    while ((one_modem_tuple = g_variant_iter_next_value(&array_iter))) {
        if (!g_variant_is_of_type(one_modem_tuple, G_VARIANT_TYPE_TUPLE)
            || g_variant_n_children(one_modem_tuple) != 2)
        {
            std::cerr << "Modem item is not (o, a{...}). Skipping.\n";
            g_variant_unref(one_modem_tuple);
            continue;
        }

        GVariant* path_var = g_variant_get_child_value(one_modem_tuple, 0);
        const char* modem_path = g_variant_get_string(path_var, nullptr);
        std::cout << "Modem path: " << modem_path << std::endl;
        g_variant_unref(path_var);

        GVariant* properties_dict = g_variant_get_child_value(one_modem_tuple, 1);
        if (!properties_dict || !g_variant_is_of_type(properties_dict, G_VARIANT_TYPE_DICTIONARY)) {
            std::cerr << "Modem properties not a dictionary.\n";
            if (properties_dict) g_variant_unref(properties_dict);
            g_variant_unref(one_modem_tuple);
            continue;
        }

        GVariantIter dict_iter;
        g_variant_iter_init(&dict_iter, properties_dict);

        GVariant* dict_entry = nullptr;  
        while ((dict_entry = g_variant_iter_next_value(&dict_iter))) {
            if (!g_variant_is_of_type(dict_entry, G_VARIANT_TYPE_DICT_ENTRY)
                || g_variant_n_children(dict_entry) != 2)
            {
                std::cerr << "Dictionary entry not {s => X}.\n";
                g_variant_unref(dict_entry);
                continue;
            }

            GVariant* key_var = g_variant_get_child_value(dict_entry, 0);
            const char* prop_key = g_variant_get_string(key_var, nullptr);
            g_variant_unref(key_var);

            GVariant* val_var = g_variant_get_child_value(dict_entry, 1);

            std::cout << "  Property [" << prop_key << "]: "
                      << g_variant_print(val_var, TRUE) << std::endl;

            if (std::string(prop_key) == "Powered"
                && g_variant_is_of_type(val_var, G_VARIANT_TYPE_BOOLEAN))
            {
                bool powered = g_variant_get_boolean(val_var);
                std::cout << "    -> Powered = " << (powered ? "true" : "false") << "\n";
            }

            else if (std::string(prop_key) == "Interfaces"
                     && g_variant_is_of_type(val_var, G_VARIANT_TYPE_STRING_ARRAY))
            {
                GVariantIter str_iter;
                g_variant_iter_init(&str_iter, val_var);

                const gchar* iface = nullptr;
                bool found_vcm = false;

                while (g_variant_iter_next(&str_iter, "&s", &iface)) {
                    std::cout << "    -> Interface: " << iface << "\n";
                    if (std::string(iface) == "org.ofono.VoiceCallManager") {
                        found_vcm = true;
                    }
                }
                if (found_vcm) {
                    std::cout << "      (This modem supports VoiceCallManager.)\n";
                }
            }

            g_variant_unref(val_var);
            g_variant_unref(dict_entry);
        }
        g_variant_unref(properties_dict);
        g_variant_unref(one_modem_tuple);
    }

    g_variant_unref(modems_array);
    g_variant_unref(result);


    g_dbus_connection_signal_subscribe(
        connection,
        "org.ofono",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        nullptr,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        [](GDBusConnection* /*conn*/,
           const gchar* /*sender*/,
           const gchar* object_path,
           const gchar* interface_name,
           const gchar* /*signal_name*/,
           GVariant* parameters,
           gpointer user_data)
        {
            auto* self = static_cast<BluetoothMedia*>(user_data);
            self->handle_call_signal(object_path, parameters);
        },
        this,
        nullptr
    );
    std::cout << "Subscribed to PropertiesChanged (org.ofono) globally.\n";

    g_dbus_connection_signal_subscribe(
    connection,
    /* bus name = */ "org.ofono",
    /* interface name = */ "org.ofono.VoiceCallManager",
    /* signal name = */ "CallAdded",
    /* object_path = */ nullptr, // listen on any modem
    /* arg0 = */ nullptr,
    G_DBUS_SIGNAL_FLAGS_NONE,
    // callback function:
    [](GDBusConnection* /*conn*/,
       const gchar* /*sender*/,
       const gchar* object_path,
       const gchar* /*interface_name*/,
       const gchar* /*signal_name*/,
       GVariant* parameters,
       gpointer user_data)
    {
        auto* self = static_cast<BluetoothMedia*>(user_data);
        self->on_call_added(object_path, parameters);
    },
    this,
    nullptr
);
   std::cout << "Subscribed to CallAdded (org.ofono) globally.\n";

g_dbus_connection_signal_subscribe(
    connection,
    "org.ofono",
    "org.ofono.VoiceCallManager",
    "CallRemoved",
    /* object_path = */ nullptr,
    /* arg0 = */ nullptr,
    G_DBUS_SIGNAL_FLAGS_NONE,
    [](GDBusConnection* /*conn*/,
       const gchar* /*sender*/,
       const gchar* object_path,
       const gchar* /*interface_name*/,
       const gchar* /*signal_name*/,
       GVariant* parameters,
       gpointer user_data)
    {
        auto* self = static_cast<BluetoothMedia*>(user_data);
        self->on_call_removed(object_path, parameters);
    },
    this,
    nullptr
);
  g_dbus_connection_signal_subscribe(
        connection,
        "org.ofono",
        "org.freedesktop.DBus.ObjectManager",
        "InterfacesRemoved",
        nullptr,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        [](GDBusConnection* /*conn*/,
           const gchar* /*sender*/,
           const gchar* object_path,
           const gchar* /*interface_name*/,
           const gchar* /*signal_name*/,
           GVariant* parameters,
           gpointer user_data)
        {
            auto* self = static_cast<BluetoothMedia*>(user_data);
            self->handle_interfaces_removed(object_path, parameters);
        },
        this,
        nullptr
    );

        g_dbus_connection_signal_subscribe(
        connection,
        "org.bluez",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        nullptr,
        "org.bluez.Device1",
        G_DBUS_SIGNAL_FLAGS_NONE,
        [](GDBusConnection* /*conn*/,
        const gchar* /*sender*/,
        const gchar* object_path,
        const gchar* interface_name,
        const gchar* /*signal_name*/,
        GVariant* parameters,
        gpointer user_data)
        {
            // You can inspect the changed properties
            GVariantIter *iter = nullptr;
            const gchar *changed_iface = nullptr;
            g_variant_get(parameters, "(sa{sv}as)", &changed_iface, &iter, nullptr);
            if (std::string(changed_iface) == "org.bluez.Device1") {
                const gchar* prop = nullptr;
                GVariant* value = nullptr;
                while (g_variant_iter_next(iter, "{sv}", &prop, &value)) {
                    if (std::string(prop) == "Connected") {
                        bool connected = g_variant_get_boolean(value);
                        if (!connected) {
                            std::cout << "Device " << object_path << " disconnected." << std::endl;
                            // Set your internal flag or clear cached paths as needed.
                        }
                    }
                    g_variant_unref(value);
                }
                g_variant_iter_free(iter);
            }
        },
        this,
        nullptr
    );


    std::cout << "Subscribed to InterfacesRemoved signals.\n";
    g_dbus_connection_signal_subscribe(
        connection,
        "org.ofono",
        "org.freedesktop.DBus.ObjectManager",
        "InterfacesAdded",
        nullptr,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        [](GDBusConnection* /*conn*/,
           const gchar* /*sender*/,
           const gchar* object_path,
           const gchar* /*interface_name*/,
           const gchar* /*signal_name*/,
           GVariant* parameters,
           gpointer user_data)
        {
            auto* self = static_cast<BluetoothMedia*>(user_data);
            self->handle_interfaces_added(parameters);
        },
        this,
        nullptr
    );
    std::cout << "Subscribed to InterfacesAdded for new calls.\n";
}

void BluetoothMedia::on_call_added(const gchar* manager_path, GVariant* parameters)
{
    // (o a{sv}) => call_path, dictionary of props
    if (!parameters || !g_variant_is_of_type(parameters, G_VARIANT_TYPE_TUPLE) 
        || g_variant_n_children(parameters) < 2) {
        return;
    }

    GVariant* path_variant = g_variant_get_child_value(parameters, 0);
    const char* call_path = g_variant_get_string(path_variant, nullptr);
    g_variant_unref(path_variant);

    GVariant* props_variant = g_variant_get_child_value(parameters, 1);

    std::cout << "[on_call_added] manager_path=" << manager_path << std::endl;
    std::cout << "New call object path: " << (call_path ? call_path : "(null)") << std::endl;

    if (!props_variant || !g_variant_is_of_type(props_variant, G_VARIANT_TYPE_DICTIONARY)) {
        if (props_variant) g_variant_unref(props_variant);
        return;
    }

    GVariantIter dict_iter;
    g_variant_iter_init(&dict_iter, props_variant);

    GVariant* entry = nullptr;
    bool is_incoming = false;
    std::string call_state;
    std::string line_id;
    std::string display_name;      // from "Name"
    bool is_multiparty = false;
    bool is_emergency = false;

    while ((entry = g_variant_iter_next_value(&dict_iter))) {
        if (!g_variant_is_of_type(entry, G_VARIANT_TYPE_DICT_ENTRY) 
            || g_variant_n_children(entry) != 2)
        {
            g_variant_unref(entry);
            continue;
        }

        // child(0) => key (string)
        GVariant* key_var = g_variant_get_child_value(entry, 0);
        const char* key_str = g_variant_get_string(key_var, nullptr);
        g_variant_unref(key_var);

        // child(1) => v(...)
        GVariant* outer_val = g_variant_get_child_value(entry, 1);
        g_variant_unref(entry);

        if (!outer_val || !g_variant_is_of_type(outer_val, G_VARIANT_TYPE_VARIANT)) {
            if (outer_val) g_variant_unref(outer_val);
            continue;
        }

        // unbox the real type
        GVariant* unboxed = g_variant_get_variant(outer_val);
        g_variant_unref(outer_val);

        // parse known properties:
        if (std::string(key_str) == "State"
            && g_variant_is_of_type(unboxed, G_VARIANT_TYPE_STRING))
        {
            const char* st = g_variant_get_string(unboxed, nullptr);
            call_state = (st ? st : "");
            if (call_state == "incoming") {
                is_incoming = true;
            }
        }
        else if (std::string(key_str) == "LineIdentification"
                 && g_variant_is_of_type(unboxed, G_VARIANT_TYPE_STRING))
        {
            const char* num = g_variant_get_string(unboxed, nullptr);
            if (num) line_id = num;
        }
        else if (std::string(key_str) == "Name"
                 && g_variant_is_of_type(unboxed, G_VARIANT_TYPE_STRING))
        {
            const char* nm = g_variant_get_string(unboxed, nullptr);
            if (nm) display_name = nm;
        }
        else if (std::string(key_str) == "Multiparty"
                 && g_variant_is_of_type(unboxed, G_VARIANT_TYPE_BOOLEAN))
        {
            is_multiparty = g_variant_get_boolean(unboxed);
        }
        else if (std::string(key_str) == "Emergency"
                 && g_variant_is_of_type(unboxed, G_VARIANT_TYPE_BOOLEAN))
        {
            is_emergency = g_variant_get_boolean(unboxed);
        }
        // ... also "RemoteHeld", "RemoteMultiparty", etc. if you want them

        g_variant_unref(unboxed);
    }
    g_variant_unref(props_variant);

    // store globally
    g_current_call_path = (call_path ? call_path : "");
    g_current_caller_number = line_id;
    g_call_in_progress = (call_state == "incoming" || call_state == "active");

    // Print
    if (is_incoming) {
        std::cout << "==> [CallAdded] Incoming call on " << g_current_call_path
                  << " from " << (line_id.empty() ? "Unknown" : line_id)
                  << " (state=" << call_state << ")" << std::endl;
    } else {
        std::cout << "[CallAdded] New call " << g_current_call_path
                  << " (state=" << call_state << ")\n";
    }

    if (!display_name.empty()) {
        std::cout << "   Name: " << display_name << "\n";
    }
    if (is_multiparty) {
        std::cout << "   This call is multiparty (conference).\n";
    }
    if (is_emergency) {
        std::cout << "   This call is flagged as emergency.\n";
    }

}

void BluetoothMedia::handle_interfaces_added(GVariant* parameters) {
    const char* object_path = nullptr;
    GVariant* interfaces = nullptr;
    g_variant_get(parameters, "(oa{sa{sv}})", &object_path, &interfaces);

    GVariantIter* iface_iter = nullptr;
    g_variant_get(interfaces, "a{sa{sv}}", &iface_iter);

    const char* interface_name = nullptr;
    GVariant* props_dict = nullptr;

    bool has_manager = false;

    while (g_variant_iter_next(iface_iter, "{sa{sv}}", &interface_name, &props_dict)) {
        std::cout << "Interface added: " << interface_name
                  << " on path: " << object_path << std::endl;

        if (std::string(interface_name) == "org.ofono.VoiceCallManager") {
            has_manager = true;
        }
        else if (std::string(interface_name) == "org.ofono.VoiceCall") {
            std::cout << "New VoiceCall object at path: " << object_path << std::endl;
        }

        g_variant_unref(props_dict);
    }
    g_variant_iter_free(iface_iter);
    g_variant_unref(interfaces);

    if (has_manager) {
        std::cout << "New VoiceCallManager added at " << object_path
                  << ", polling calls.\n";
        poll_ofono_calls();
    }
}


void BluetoothMedia::handle_call_signal(const gchar* object_path, GVariant* parameters) {


    std::cout << "Handling PropertiesChanged for object_path: " << object_path << std::endl;

    const gchar* changed_interface = nullptr;
    GVariantIter* changed_props_iter = nullptr; 
    GVariant* invalidated_props = nullptr;

    g_variant_get(
        parameters, 
        "(sa{sv}as)", 
        &changed_interface, 
        &changed_props_iter, 
        &invalidated_props
    );

    if (std::string(changed_interface) != "org.ofono.VoiceCall") {
        std::cout << "Ignoring interface change: " << changed_interface << std::endl;
        g_variant_iter_free(changed_props_iter);
        g_variant_unref(invalidated_props);
        return;
    }

    const gchar* property_name = nullptr;
    GVariant* property_value = nullptr;

    std::string old_state;
    {
        auto it = call_states.find(object_path);
        if (it != call_states.end()) {
            old_state = it->second;
        }
    }

    while (g_variant_iter_next(changed_props_iter, "{sv}", &property_name, &property_value)) {
        if (std::string(property_name) == "State") {
            const gchar* new_state_cstr = nullptr;
            g_variant_get(property_value, "&s", &new_state_cstr);
            std::string new_state = (new_state_cstr ? new_state_cstr : "");
            

            std::cout << "Call State changed for " << object_path 
                      << " => " << new_state << std::endl;
            if (new_state == "terminated") {
                    std::cout << "Call at path " << object_path << " is terminated.\n";

                    // remove from map
                    auto it = call_states.find(object_path);
                    if (it != call_states.end()) {
                        call_states.erase(it);
                    }

                    // if it was current call
                    if (g_current_call_path == object_path) {
                        g_call_in_progress = false;
                        g_current_call_path.clear();
                        g_current_caller_number.clear();
                    }
                }
            // If the new state is "incoming" and wasn't "incoming" before
             else if (new_state == "incoming" && old_state != "incoming") {
                std::cout << "Incoming call detected on path: " 
                          << object_path << std::endl;
            }
            // If we were previously incoming, but now no longer incoming
            else if (old_state == "incoming" && new_state != "incoming") {
                std::cout << "Call at path " << object_path 
                          << " is no longer incoming (new state: " << new_state 
                          << ")" << std::endl;
            }

            // Update the stored state
            call_states[object_path] = new_state;
        }

        g_variant_unref(property_value);
    }

    g_variant_iter_free(changed_props_iter);
    g_variant_unref(invalidated_props);
}









bool BluetoothMedia::supports_voice_call_manager(GVariant* interfaces_dict) {
    if (!interfaces_dict) {
        std::cerr << "supports_voice_call_manager: interfaces_dict is NULL." << std::endl;
        return false;
    }

    GVariantIter* iface_iter;
    g_variant_get(interfaces_dict, "a{sa{sv}}", &iface_iter);

    const char* interface_name;
    GVariant* dummy = nullptr;
    while (g_variant_iter_next(iface_iter, "{sa{sv}}", &interface_name, &dummy)) {
        g_variant_unref(dummy);
        if (std::string(interface_name) == "org.ofono.VoiceCallManager") {
            g_variant_iter_free(iface_iter);
            return true;
        }
    }
    g_variant_iter_free(iface_iter);
    return false;
}



// Static VTable definition for BluetoothAgent
const GDBusInterfaceVTable BluetoothAgent::vtable = {
    BluetoothAgent::handle_method_call,
    nullptr,
    nullptr,
    {0}
};





BluetoothAgent::BluetoothAgent(GDBusConnection* conn, const std::string& object_path)
    : connection(conn), agent_path(object_path), registration_id(0), introspection_data(nullptr) {

    // Define the interface XML
    const char* agent_interface_xml =
        "<node>"
        "  <interface name='org.bluez.Agent1'>"
        "    <method name='Release'/>"
        "    <method name='RequestPinCode'>"
        "      <arg direction='in' type='o' name='device'/>"
        "      <arg direction='out' type='s' name='pincode'/>"
        "    </method>"
        "    <method name='RequestPasskey'>"
        "      <arg direction='in' type='o' name='device'/>"
        "      <arg direction='out' type='u' name='passkey'/>"
        "    </method>"
        "    <method name='DisplayPasskey'>"
        "      <arg direction='in' type='o' name='device'/>"
        "      <arg direction='in' type='u' name='passkey'/>"
        "      <arg direction='in' type='q' name='entered'/>"
        "    </method>"
        "    <method name='RequestConfirmation'>"
        "      <arg direction='in' type='o' name='device'/>"
        "      <arg direction='in' type='u' name='passkey'/>"
        "    </method>"
        "    <method name='AuthorizeService'>"
        "      <arg direction='in' type='o' name='device'/>"
        "      <arg direction='in' type='s' name='uuid'/>"
        "    </method>"
        "    <signal name='Canceled'/>"
        "  </interface>"
        "</node>";

    GError* error = nullptr;
    introspection_data = g_dbus_node_info_new_for_xml(agent_interface_xml, &error);
    if (error) {
        std::cerr << "Failed to parse introspection XML: " << error->message << std::endl;
        g_error_free(error);
        throw std::runtime_error("Introspection XML parsing failed.");
    }

    // Register the object on D-Bus
    registration_id = g_dbus_connection_register_object(
        connection,
        agent_path.c_str(),
        introspection_data->interfaces[0],
        &vtable,
        this, // user_data
        nullptr, // user_data_free_func
        &error
    );

    if (registration_id == 0) {
        std::cerr << "Failed to register agent object: " << error->message << std::endl;
        g_error_free(error);
        throw std::runtime_error("Agent object registration failed.");
    }

    std::cout << "BluetoothAgent object registered at path: " << agent_path << std::endl;
}

BluetoothAgent::~BluetoothAgent() {
    unregister_agent();
    if (registration_id > 0) {
        g_dbus_connection_unregister_object(connection, registration_id);
    }
    if (introspection_data) {
        g_dbus_node_info_unref(introspection_data);
    }
}

// void BluetoothAgent::handle_method_call(
//     GDBusConnection* connection,
//     const gchar* sender,
//     const gchar* object_path,
//     const gchar* interface_name,
//     const gchar* method_name,
//     GVariant* parameters,
//     GDBusMethodInvocation* invocation,
//     gpointer user_data
// ) {
//     BluetoothMedia* media = static_cast<BluetoothMedia*>(user_data);

//     std::lock_guard<std::mutex> lock(media->pairing_mutex);

//     if (g_strcmp0(method_name, "RequestPinCode") == 0) {
//         const char* device;
//         g_variant_get(parameters, "(&o)", &device);

//         std::cout << "[Bluetooth] Incoming pairing request from: " << device << std::endl;

//         media->current_pairing_request = {
//             .device_path = device,
//             .passkey = "PIN_REQUIRED",
//             .requires_confirmation = false,
//             .active = true,
//             .respond = [invocation](bool accept) {
//                 if (accept) {
//                     g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", "1234"));
//                 } else {
//                     g_dbus_method_invocation_return_dbus_error(invocation, "org.bluez.Error.Rejected", "User rejected pairing.");
//                 }
//             }
//         };
//     }
//     else if (g_strcmp0(method_name, "RequestConfirmation") == 0) {
//         const char* device;
//         uint32_t passkey;
//         g_variant_get(parameters, "(&ou)", &device, &passkey);

//         std::cout << "[Bluetooth] Incoming pairing request from: " << device << std::endl;
//         std::cout << "Passkey: " << passkey << std::endl;

//         media->current_pairing_request = {
//             .device_path = device,
//             .passkey = std::to_string(passkey),
//             .requires_confirmation = true,
//             .active = true,
//             .respond = [invocation](bool accept) {
//                 if (accept) {
//                     g_dbus_method_invocation_return_value(invocation, nullptr);
//                 } else {
//                     g_dbus_method_invocation_return_dbus_error(invocation, "org.bluez.Error.Rejected", "User rejected pairing.");
//                 }
//             }
//         };
//     }
//     else {
//         std::cerr << "[Bluetooth] Unknown method: " << method_name << std::endl;
        

//     }
// }
void BluetoothAgent::handle_method_call(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* method_name,
    GVariant* parameters,
    GDBusMethodInvocation* invocation,
    gpointer user_data
) {
    BluetoothAgent* self = static_cast<BluetoothAgent*>(user_data);

    if (g_strcmp0(method_name, "Release") == 0) {
        std::cout << "Agent Release called" << std::endl;
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
    else if (g_strcmp0(method_name, "RequestPinCode") == 0) {
        const char* device;
        g_variant_get(parameters, "(&o)", &device);
        std::cout << "RequestPinCode called for device: " << device << std::endl;
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", "1234"));
    }
    else if (g_strcmp0(method_name, "RequestPasskey") == 0) {
        const char* device;
        g_variant_get(parameters, "(&o)", &device);
        std::cout << "RequestPasskey called for device: " << device << std::endl;
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", 123456));
    }
    else if (g_strcmp0(method_name, "DisplayPasskey") == 0) {
        const char* device;
        uint32_t passkey;
        uint16_t entered;
        g_variant_get(parameters, "(&ouq)", &device, &passkey, &entered);
        std::cout << "DisplayPasskey called for device: " << device 
                  << ", Passkey: " << passkey 
                  << ", Entered: " << entered << std::endl;
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
    else if (g_strcmp0(method_name, "RequestConfirmation") == 0) {
        const char* device;
        uint32_t passkey;
        g_variant_get(parameters, "(&ou)", &device, &passkey);
        std::cout << "RequestConfirmation called for device: " << device 
                  << ", Passkey: " << passkey << std::endl;
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
    else if (g_strcmp0(method_name, "AuthorizeService") == 0) {
        const char* device;
        const char* uuid;
        g_variant_get(parameters, "(&os)", &device, &uuid);
        std::cout << "AuthorizeService called for device: " << device 
                  << ", UUID: " << uuid << std::endl;
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
    else {
        std::cerr << "Unknown method called: " << method_name << std::endl;
        g_dbus_method_invocation_return_error(invocation,
            G_IO_ERROR,
            G_IO_ERROR_NOT_SUPPORTED,
            "Method '%s' is not supported",
            method_name);
    }
}

void BluetoothMedia::accept_pairing() {
    std::lock_guard<std::mutex> lock(pairing_mutex);

    if (current_pairing_request.active && current_pairing_request.respond) {
        current_pairing_request.respond(true);
        std::cout << "[Bluetooth] Pairing accepted for " << current_pairing_request.device_path << std::endl;
        current_pairing_request.active = false;
    }
}

void BluetoothMedia::reject_pairing() {
    std::lock_guard<std::mutex> lock(pairing_mutex);

    if (current_pairing_request.active && current_pairing_request.respond) {
        current_pairing_request.respond(false);
        std::cout << "[Bluetooth] Pairing rejected for " << current_pairing_request.device_path << std::endl;
        current_pairing_request.active = false;
    }
}


void BluetoothAgent::register_agent() {
    GError* error = nullptr;

    // RegisterAgent
    g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/org/bluez",
        "org.bluez.AgentManager1",
        "RegisterAgent",
        g_variant_new("(os)", agent_path.c_str(), "KeyboardDisplay"),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to register agent: " << error->message << std::endl;
        g_error_free(error);
        throw std::runtime_error("Agent registration failed.");
    }

    // RequestDefaultAgent
    g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/org/bluez",
        "org.bluez.AgentManager1",
        "RequestDefaultAgent",
        g_variant_new("(o)", agent_path.c_str()),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to set default agent: " << error->message << std::endl;
        g_error_free(error);
        throw std::runtime_error("Failed to set default agent.");
    }

    std::cout << "Bluetooth agent registered and set as default." << std::endl;
}

void BluetoothAgent::unregister_agent() {
    GError* error = nullptr;

    g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/org/bluez",
        "org.bluez.AgentManager1",
        "UnregisterAgent",
        g_variant_new("(o)", agent_path.c_str()),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to unregister agent: " << error->message << std::endl;
        g_error_free(error);
    }
    else {
        std::cout << "Bluetooth agent unregistered successfully." << std::endl;
    }
}
// Find the Bluetooth adapter path
std::string BluetoothMedia::find_adapter_path() {
    GError* error = nullptr;

    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        nullptr,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to get managed objects: " << error->message << std::endl;
        g_error_free(error);
        return "";
    }

    GVariantIter* objects;
    const char* object_path;
    GVariantIter* interfaces;

    g_variant_get(result, "(a{oa{sa{sv}}})", &objects);

    while (g_variant_iter_next(objects, "{oa{sa{sv}}}", &object_path, &interfaces)) {
        const char* interface_name;

        while (g_variant_iter_next(interfaces, "{sa{sv}}", &interface_name, nullptr)) {
            if (std::string(interface_name) == "org.bluez.Adapter1") {
                g_variant_iter_free(interfaces);
                g_variant_iter_free(objects);
                g_variant_unref(result);
                return std::string(object_path);
            }
        }
        g_variant_iter_free(interfaces);
    }

    g_variant_iter_free(objects);
    g_variant_unref(result);

    std::cerr << "No Bluetooth adapter found." << std::endl;
    return "";
}

// Find the MediaPlayer1 path
std::string BluetoothMedia::find_media_player_path() {
    GError* error = nullptr;

    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        nullptr,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to get managed objects: " << error->message << std::endl;
        g_error_free(error);
        return "";
    }

    GVariantIter* objects;
    const char* object_path;
    GVariantIter* interfaces;

    g_variant_get(result, "(a{oa{sa{sv}}})", &objects);

    while (g_variant_iter_next(objects, "{oa{sa{sv}}}", &object_path, &interfaces)) {
        const char* interface_name;

        while (g_variant_iter_next(interfaces, "{sa{sv}}", &interface_name, nullptr)) {
            if (std::string(interface_name) == "org.bluez.MediaPlayer1") {
                // Found a MediaPlayer1 object
                std::cout << "Found MediaPlayer1 at path: " << object_path << std::endl;
                g_variant_iter_free(interfaces);
                g_variant_iter_free(objects);
                g_variant_unref(result);
                return std::string(object_path);
            }
        }
        g_variant_iter_free(interfaces);
    }

    g_variant_iter_free(objects);
    g_variant_unref(result);

    std::cout << "No MediaPlayer1 objects found." << std::endl;
    return "";
}


void BluetoothMedia::setup_bluetooth() {
    // Existing setup for Bluetooth (power on, pairable, etc.)
    if (std::system("bluetoothctl power on") != 0) {
        throw std::runtime_error("Failed to power on Bluetooth.");
    }
    if (std::system("bluetoothctl pairable on") != 0) {
        throw std::runtime_error("Failed to set Bluetooth to pairable mode.");
    }
    if (std::system("bluetoothctl discoverable on") != 0) {
        throw std::runtime_error("Failed to make Bluetooth discoverable.");
    }

    std::cout << "Bluetooth powered on, pairable, and discoverable." << std::endl;

    const std::string agent_path = "/com/yourapp/agent"; // Use a unique path
    BluetoothAgent* agent = new BluetoothAgent(connection, agent_path);

    try {
        agent->register_agent();
    }
    catch (const std::exception& e) {
        std::cerr << "Agent registration exception: " << e.what() << std::endl;
        throw;
    }

    // Subscribe to D-Bus signals related to calls (already done in monitor_calls):
    monitor_calls();

    // Initialize and run the main loop in a separate thread
    main_loop = g_main_loop_new(nullptr, FALSE);
    if (!main_loop) {
        throw std::runtime_error("Failed to initialize main loop.");
    }
    g_timeout_add_seconds(3, [](gpointer user_data) -> gboolean {
        BluetoothMedia* self = static_cast<BluetoothMedia*>(user_data);
        self->poll_ofono_calls();
        // returning TRUE means keep calling
        return TRUE;
    }, this);
    std::thread loop_thread([this]() {
        std::cout << "Main loop thread started." << std::endl;
        g_main_loop_run(main_loop);
        std::cout << "Main loop thread exited." << std::endl;
    });
    loop_thread.detach();
}


// Signal handler for property changes
void BluetoothMedia::on_properties_changed(
    GDBusConnection* conn, const gchar* sender_name,
    const gchar* object_path, const gchar* interface_name,
    const gchar* signal_name, GVariant* parameters, gpointer user_data) {

    if (std::string(interface_name) == "org.bluez.MediaPlayer1") {
        // MediaPlayer1 properties have changed
        std::cout << "PropertiesChanged signal received for MediaPlayer1 at path: " << object_path << std::endl;
        // Optionally, you can update media_player_path or handle specific property changes here
    }
}

// Register to listen for PropertiesChanged signals
void BluetoothMedia::start_media_monitor() {
    g_dbus_connection_signal_subscribe(
        connection,
        "org.bluez",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        nullptr,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_properties_changed,
        this,
        nullptr
    );
    std::cout << "Media monitor started, listening for property changes." << std::endl;
}

// Trust connected devices
void BluetoothMedia::trust_connected_device() {
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        nullptr,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to get managed objects: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    GVariantIter* objects;
    const char* object_path;
    GVariantIter* interfaces;

    g_variant_get(result, "(a{oa{sa{sv}}})", &objects);

    while (g_variant_iter_next(objects, "{oa{sa{sv}}}", &object_path, &interfaces)) {
        const char* interface_name;
        GVariantIter* properties;

        while (g_variant_iter_next(interfaces, "{sa{sv}}", &interface_name, &properties)) {
            if (std::string(interface_name) == "org.bluez.Device1") {
                bool connected = false;
                const char* device_address = nullptr;

                // Declare the variables before using them
                const char* property_name = nullptr;
                GVariant* property_value = nullptr;

                while (g_variant_iter_next(properties, "{sv}", &property_name, &property_value)) {
                    if (std::string(property_name) == "Connected") {
                        connected = g_variant_get_boolean(property_value);
                    }
                    else if (std::string(property_name) == "Address") {
                        device_address = g_variant_dup_string(property_value, nullptr);
                    }
                    g_variant_unref(property_value);
                }

                if (connected && device_address) {
                    std::string command = "bluetoothctl trust " + std::string(device_address);
                    if (std::system(command.c_str()) == 0) {
                        std::cout << "Trusted connected device: " << device_address << std::endl;
                    }
                    else {
                        std::cerr << "Failed to trust device: " << device_address << std::endl;
                    }
                    g_free((gpointer)device_address);
                }

                g_variant_iter_free(properties);
            }
        }
        g_variant_iter_free(interfaces);
    }

    g_variant_iter_free(objects);
    g_variant_unref(result);
}

// Method to check and set MediaPlayer1 path dynamically
bool BluetoothMedia::check_and_set_media_player_path() {
    if (!media_player_path.empty()) {
        return true;
    }

    // Implement backoff: only attempt to find media player path if enough time has passed since last attempt
    auto now = std::chrono::steady_clock::now();
    if (now - last_retry_time < std::chrono::milliseconds(retry_interval_ms)) {
        return false; // Skip retrying too soon
    }
    last_retry_time = now;

    media_player_path = find_media_player_path();
    if (!media_player_path.empty()) {
        std::cout << "MediaPlayer path set to: " << media_player_path << std::endl;
        return true;
    }
    else {
        std::cerr << "MediaPlayer not found. Will retry after backoff interval." << std::endl;
        return false;
    }
}

// Get media information
MediaInfo BluetoothMedia::get_media_info()  {
     MediaInfo info = {"", "", 0, 0, "paused"};

    if (media_player_path.empty()) {
        std::cout << "No MediaPlayer path available." << std::endl;
        return info;
    }

    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        media_player_path.c_str(),
        "org.freedesktop.DBus.Properties",
        "GetAll",
        g_variant_new("(s)", "org.bluez.MediaPlayer1"),
        G_VARIANT_TYPE("(a{sv})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

if (!result) {
    std::cerr << "Failed to get media info: " << (error && error->message ? error->message : "Unknown error") << std::endl;
    if (error && error->message) {
        // Detect if the error is due to an unknown object and reset media_player_path
        if (g_str_has_prefix(error->message, "org.freedesktop.DBus.Error.UnknownObject")) {
            std::cerr << "MediaPlayer object no longer exists. Resetting media_player_path." << std::endl;
            media_player_path.clear();
            is_disconnected.store(true);
        }
        g_error_free(error);
    }
    return info;
}

    GVariantIter* properties;
    const char* key;
    GVariant* value;

    g_variant_get(result, "(a{sv})", &properties);

    while (g_variant_iter_next(properties, "{sv}", &key, &value)) {
        if (std::string(key) == "Track") {
            const char* title = nullptr;
            const char* artist = nullptr;
            guint32 duration_ms = 0;



            // Extract 'Title', 'Artist', and 'Duration' from the 'Track' dictionary
            g_variant_lookup(value, "Title", "&s", &title);
            if (g_variant_lookup(value, "Artist", "&s", &artist) && artist && strlen(artist) > 0) {
                    info.artist = std::string(artist);
            }
            g_variant_lookup(value, "Duration", "u", &duration_ms);

            if (title) {
                info.title = std::string(title);
            }
            if (artist) {
                info.artist = std::string(artist);
            }
            info.duration = static_cast<int>(duration_ms / 1000); // Convert ms to seconds
        } else if (std::string(key) == "Position") {
            guint32 position_ms = 0;
            g_variant_get(value, "u", &position_ms); // Correct type is 'u'

            if (info.duration > 0) {
                int position_sec = static_cast<int>(position_ms / 1000); // Convert ms to seconds
                info.progress = static_cast<int>((static_cast<double>(position_sec) / info.duration) * 100);
                if (info.progress > 100) info.progress = 100;
                if (info.progress < 0) info.progress = 0;
            }
        } else if (std::string(key) == "Status") {
            const char* status_str = nullptr;
            g_variant_get(value, "&s", &status_str);
            if (status_str) {
                info.status = std::string(status_str);
            }
        }
        g_variant_unref(value);
    }
    last_media_info = info;
    g_variant_iter_free(properties);
    g_variant_unref(result);

    return info;
}



int BluetoothMedia::get_song_progress()  {
     if (media_player_path.empty()) {
        std::cerr << "No MediaPlayer path available.\n";
        return 0;
    }

    GError* error = nullptr;

    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        media_player_path.c_str(),
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.bluez.MediaPlayer1", "Position"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

 if (!result) {
    std::cerr << "Failed to get Position property: " << (error && error->message ? error->message : "Unknown error") << std::endl;
    if (error && error->message) {
        // Detect if the error is due to an unknown object and reset media_player_path
        if (g_str_has_prefix(error->message, "org.freedesktop.DBus.Error.UnknownObject")) {
            std::cerr << "MediaPlayer object no longer exists. Resetting media_player_path." << std::endl;
            media_player_path.clear();
            is_disconnected.store(true);
        }
        g_error_free(error);
    }
    return 0;
}

    // Validate result type
    if (!g_variant_is_of_type(result, G_VARIANT_TYPE("(v)"))) {
        std::cerr << "Invalid result type for Position: " 
                  << g_variant_get_type_string(result) << std::endl;
        g_variant_unref(result);
        return 0;
    }

    GVariant* position_variant;
    g_variant_get(result, "(v)", &position_variant);

    if (!g_variant_is_of_type(position_variant, G_VARIANT_TYPE("u"))) {
        std::cerr << "Invalid position variant type: " 
                  << g_variant_get_type_string(position_variant) << std::endl;
        g_variant_unref(position_variant);
        g_variant_unref(result);
        return 0;
    }

    guint32 position_ms = 0;
    g_variant_get(position_variant, "u", &position_ms);

    g_variant_unref(position_variant);
    g_variant_unref(result);

    int duration = get_song_duration();
    if (duration == 0) {
        return 0;
    }

    int position_sec = static_cast<int>(position_ms / 1000);
    int progress = static_cast<int>((static_cast<double>(position_sec) / duration) * 100);

    if (progress > 100) progress = 100;
    if (progress < 0) progress = 0;
    last_song_progress = progress;
    return progress;
}



int BluetoothMedia::get_song_duration() const {
    if (media_player_path.empty()) {
        std::cerr << "No MediaPlayer path available." << std::endl;
        return 0;
    }

    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        media_player_path.c_str(),
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.bluez.MediaPlayer1", "Track"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to get Track property: " << error->message << std::endl;
        g_error_free(error);
        return 0;
    }


    if (!g_variant_is_of_type(result, G_VARIANT_TYPE("(v)"))) {
        std::cerr << "Invalid result type for Track: " 
                  << g_variant_get_type_string(result) << std::endl;
        g_variant_unref(result);
        return 0;
    }

    GVariant* track_variant;
    g_variant_get(result, "(v)", &track_variant);


    if (!g_variant_is_of_type(track_variant, G_VARIANT_TYPE("a{sv}"))) {
        std::cerr << "Invalid track variant type: " 
                  << g_variant_get_type_string(track_variant) << std::endl;
        g_variant_unref(track_variant);
        g_variant_unref(result);
        return 0;
    }

    guint32 duration_ms = 0;
    bool duration_found = g_variant_lookup(track_variant, "Duration", "u", &duration_ms);

    if (!duration_found) {
        std::cerr << "DEBUG: Duration property not found in Track data." << std::endl;
    }

    g_variant_unref(track_variant);
    g_variant_unref(result);

    return static_cast<int>(duration_ms / 1000); 
}




void BluetoothMedia::play_pause() {
    if (media_player_path.empty()) {
        std::cerr << "No MediaPlayer path available.\n";
        return;
    }

    GError* error = nullptr;

    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        media_player_path.c_str(),
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.bluez.MediaPlayer1", "Status"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to get Status property: " << error->message << "\n";
        g_error_free(error);
        return;
    }

    GVariant* status_variant;
    g_variant_get(result, "(v)", &status_variant);

    const char* status = nullptr;
    g_variant_get(status_variant, "&s", &status);

    if (strcmp(status, "playing") == 0) {
        // Pause playback
        GVariant* pause_result = g_dbus_connection_call_sync(
            connection,
            "org.bluez",
            media_player_path.c_str(),
            "org.bluez.MediaPlayer1",
            "Pause",
            nullptr,
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            &error
        );

        if (!pause_result) {
            std::cerr << "Failed to pause playback: " << error->message << "\n";
            g_error_free(error);
        } else {
            g_variant_unref(pause_result);
        }
    } else {
        // Start playback
        GVariant* play_result = g_dbus_connection_call_sync(
            connection,
            "org.bluez",
            media_player_path.c_str(),
            "org.bluez.MediaPlayer1",
            "Play",
            nullptr,
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            &error
        );

        if (!play_result) {
            std::cerr << "Failed to start playback: " << error->message << "\n";
            g_error_free(error);
        } else {
            g_variant_unref(play_result);
        }
    }

    g_variant_unref(status_variant);
    g_variant_unref(result);
}


// Skip to the next track
void BluetoothMedia::next_track() {
    if (media_player_path.empty()) {
        std::cerr << "No MediaPlayer path available." << std::endl;
        return;
    }

    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        media_player_path.c_str(),
        "org.bluez.MediaPlayer1",
        "Next",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to skip to next track: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    g_variant_unref(result);
    std::cout << "Skipped to next track." << std::endl;
}

// Return to the previous track
void BluetoothMedia::previous_track() {
    if (media_player_path.empty()) {
        std::cerr << "No MediaPlayer path available." << std::endl;
        return;
    }

    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        media_player_path.c_str(),
        "org.bluez.MediaPlayer1",
        "Previous",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to return to previous track: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    g_variant_unref(result);
    std::cout << "Returned to previous track." << std::endl;
}

std::string BluetoothMedia::get_playback_status() {
    if (media_player_path.empty()) {
        std::cerr << "No MediaPlayer path available." << std::endl;
        return "unknown";
    }

    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        media_player_path.c_str(),
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.bluez.MediaPlayer1", "Status"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to get Status property: " << error->message << std::endl;
        g_error_free(error);
        return "unknown";
    }

    GVariant* status_variant;
    g_variant_get(result, "(v)", &status_variant);

    const char* status_str = nullptr;
    g_variant_get(status_variant, "&s", &status_str);

    std::string status = "unknown";
    if (status_str) {
        status = std::string(status_str);
    }

    g_variant_unref(status_variant);
    g_variant_unref(result);

    return status;
}

std::vector<std::string> BluetoothMedia::get_trusted_devices() {
    std::vector<std::string> trusted_devices;
    GError* error = nullptr;

    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        nullptr,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to get managed objects: " << (error ? error->message : "Unknown error") << std::endl;
        if (error) g_error_free(error);
        return trusted_devices;
    }

    GVariantIter* objects;
    const char* object_path;
    GVariantIter* interfaces;
    std::vector<std::pair<std::string, guint64>> device_list;

    g_variant_get(result, "(a{oa{sa{sv}}})", &objects);

    while (g_variant_iter_next(objects, "{oa{sa{sv}}}", &object_path, &interfaces)) {
        const char* interface_name;
        GVariantIter* properties;
        bool trusted = false;
        guint64 last_connected = 0;

        while (g_variant_iter_next(interfaces, "{sa{sv}}", &interface_name, &properties)) {
            if (std::string(interface_name) == "org.bluez.Device1") {
                const char* property_name = nullptr;
                GVariant* property_value = nullptr;

                while (g_variant_iter_next(properties, "{sv}", &property_name, &property_value)) {
                    if (std::string(property_name) == "Trusted") {
                        trusted = g_variant_get_boolean(property_value);
                    } else if (std::string(property_name) == "Connected") {
                        if (g_variant_get_boolean(property_value)) {
                            last_connected = g_get_real_time(); // Store current time for connected devices
                        }
                    }
                    g_variant_unref(property_value);
                }

                if (trusted) {
                    device_list.emplace_back(object_path, last_connected);
                }

                g_variant_iter_free(properties);
            }
        }
        g_variant_iter_free(interfaces);
    }

    g_variant_iter_free(objects);
    g_variant_unref(result);

    // Sort devices by last connected time (most recent first)
    std::sort(device_list.begin(), device_list.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    for (const auto& device : device_list) {
        trusted_devices.push_back(device.first);
    }

    return trusted_devices;
}
bool BluetoothMedia::remove_trusted_device(const std::string& device_path) {
    GError* error = nullptr;

    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        device_path.c_str(),
        "org.freedesktop.DBus.Properties",
        "Set",
        g_variant_new("(ssv)", "org.bluez.Device1", "Trusted", g_variant_new("b", false)),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to remove trust from device: " << (error ? error->message : "Unknown error") << std::endl;
        if (error) g_error_free(error);
        return false;
    }

    g_variant_unref(result);
    std::cout << "Device " << device_path << " removed from trusted list." << std::endl;
    return true;
}
bool BluetoothMedia::connect_to_device(const std::string& device_path) {
    GError* error = nullptr;

    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        device_path.c_str(),
        "org.bluez.Device1",
        "Connect",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        std::cerr << "Failed to connect to device: " << (error ? error->message : "Unknown error") << std::endl;
        if (error) g_error_free(error);
        return false;
    }

    g_variant_unref(result);
    std::cout << "Connected to device: " << device_path << std::endl;
    return true;
}