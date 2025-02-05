Wi-Fi Configuration and Connection:

Connects to a Wi-Fi network using the provided SSID ("StefanV") and password 
Configures a static IP (192.168.79.120) for consistent network access.
Prints connection status and IP address to the serial monitor for debugging.
Web Server Setup:

Uses the ESPAsyncWebServer library to create a lightweight HTTP server on port 80.
Serves a web page with buttons to:
Capture photos (/capture).
Display the last captured photo (/saved-photo).
Placeholder for a future plant detection feature (detectPlant()).
Camera Initialization and Configuration:

Configures the ESP32 camera settings (resolution, frame size, JPEG format).
Initializes the camera, handling possible errors in the process.
Adjusts camera sensor settings (brightness, flip, saturation) for better image quality.
File System Handling (SPIFFS):

Uses the SPIFFS file system to save captured photos as /photo.jpg.
Ensures SPIFFS is properly mounted, otherwise, restarts the ESP32 if errors occur.
Capture and Save Photos:

When the "FOTOGRAFIAZA" button is pressed on the web interface, the ESP32 captures a photo.
The photo is saved to SPIFFS and served via the /saved-photo route.
The system checks if the photo has been successfully saved.
Web Interface Features:

The served HTML page provides:
A simple photo gallery layout with buttons for capturing new photos.
JavaScript functions to handle photo capture and page reload.
Placeholder for integrating the Pl@ntNet API for plant identification (currently not implemented).
Pl@ntNet API Integration Placeholder:

Includes placeholders for future plant detection using the Pl@ntNet API (plantnet_api_key and plantnet_endpoint), but this feature is not yet functional.
