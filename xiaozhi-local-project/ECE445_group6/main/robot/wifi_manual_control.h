#ifndef ROBOT_WIFI_MANUAL_CONTROL_H_
#define ROBOT_WIFI_MANUAL_CONTROL_H_

#include <functional>
#include "esp_http_server.h"
#include "robot_types.h"

namespace robot {

// SoftAP-based manual control. Starts only when mode becomes MANUAL_MODE,
// stops when leaving. Phone joins SSID "DeskBot-XXXX" and opens http://192.168.4.1/
class WifiManualControl {
public:
    using CommandSink = std::function<void(const Command&)>;

    explicit WifiManualControl(CommandSink sink) : sink_(std::move(sink)) {}

    // Brings up SoftAP + HTTP server. Safe to call when already started.
    bool Start();
    // Stops HTTP server and tears down SoftAP (optional; you can leave AP up).
    void Stop();

    bool running() const { return running_; }

private:
    esp_err_t RegisterHandlers();
    static esp_err_t HandleRoot(httpd_req_t* req);
    static esp_err_t HandleCmd(httpd_req_t* req);
    static esp_err_t HandleStatus(httpd_req_t* req);

    CommandSink   sink_;
    httpd_handle_t http_ = nullptr;
    bool          running_ = false;
};

}  // namespace robot

#endif  // ROBOT_WIFI_MANUAL_CONTROL_H_
