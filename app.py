from flask import Flask, request, jsonify
import requests
from bs4 import BeautifulSoup as bs

app = Flask(__name__)

URL = "89.212.129.11"
# Main website URL
MAIN_WEBSITE_URL = f"http://{URL}/"  # Change this to your main website URL
ESP32_URL = f"http://{URL}/setThreshold"  # Correctly formatted URL


@app.route('/fetch', methods=['GET'])
def fetch():
    try:
        # Fetch current data from the main website
        response = requests.get(MAIN_WEBSITE_URL)
        response.raise_for_status()  # Raise an error for bad responses

        soup = bs(response.text, 'html.parser')
        results = soup.find_all("p")

        if len(results) < 3:
            return jsonify({"error": "Unexpected data format from the website."}), 500

        # Parse the data
        current_temp = results[0].text.split(": ")[1]  # Extract current temperature
        threshold_value = results[1].text.split(": ")[1]  # Extract current threshold
        relay_status = "Rele je Vklopljen" in results[2].text  # Check relay status

        # Create a response based on the fetched data
        data = {
            "current_temp": current_temp,
            "threshold": threshold_value,
            "relay_status": relay_status,
        }

        return jsonify(data), 200  # Return fetched data as JSON

    except requests.RequestException as e:
        return jsonify({"error": f"Failed to fetch data from the main website. {str(e)}"}), 500

  # Replace with the actual IP address of your ESP32


# POST request to ESP32 server /setThreshold endpoint
# curl "http://89.212.129.11/setThreshold?value=30"
@app.route('/setThreshold', methods=['GET'])
def set_threshold():
    # Get the 'value' query parameter from the request
    new_threshold = request.args.get("value")  # Extract threshold value from URL parameter

    if new_threshold is None:
        return jsonify({"error": "No threshold value provided."}), 400

    # Prepare the request to post to the ESP32 server
    threshold_params = {'value': new_threshold}

    # Post to the ESP32 server
    post_response = requests.get(ESP32_URL, params=threshold_params)

    return jsonify(post_response.json()), post_response.status_code


@app.route('/relay/<action>', methods=['GET'])
def control_relay(action):
    # Define relay control logic based on action
    if action not in ["on", "off"]:
        return jsonify({"error": "Invalid relay action."}), 400

    relay_url = f"{MAIN_WEBSITE_URL}/relay/{action}"

    try:
        relay_response = requests.get(relay_url)
        if relay_response.status_code == 200:
            return jsonify({"message": f"Relay turned {'on' if action == 'on' else 'off'} successfully!"}), 200
        else:
            return jsonify({
                               "error": f"Failed to turn relay {'on' if action == 'on' else 'off'}. Status code: {relay_response.status_code}"}), relay_response.status_code

    except requests.RequestException as e:
        return jsonify({"error": f"Failed to control relay. Error: {str(e)}"}), 500


if __name__ == '__main__':
    app.run(debug=True)
