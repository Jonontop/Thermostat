import requests
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.slider import Slider
from kivy.clock import Clock
from kivy.core.window import Window
from kivy.uix.scrollview import ScrollView

# Set the window size for better appearance (remove or comment out in mobile)
Window.size = (400, 600)  # Adjust size for desktop testing; remove for mobile deployment

class ThermostatApp(App):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.error_occurred = False  # Track if an error has occurred

    def build(self):
        self.layout = BoxLayout(orientation='vertical', padding=20, spacing=10)

        # Scroll view to handle small screen sizes
        scroll_view = ScrollView()
        scroll_layout = BoxLayout(orientation='vertical', size_hint_y=None)
        scroll_layout.bind(minimum_height=scroll_layout.setter('height'))

        # Header Label
        self.header_label = Label(
            text='Nadzornik Termostata',
            font_size='24sp',
            bold=True,
            color=(1, 1, 1, 1),  # White color
            size_hint_y=None,
            height=50
        )
        scroll_layout.add_widget(self.header_label)

        # Label to display current temperature
        self.temp_label = Label(
            text='Temperatura: -- °C',
            font_size='20sp',
            color=(1, 1, 1, 1),  # White color
            size_hint_y=None,
            height=50
        )
        scroll_layout.add_widget(self.temp_label)

        # Label to display relay status
        self.relay_label = Label(
            text='Stanje: Unknown',
            font_size='20sp',
            color=(1, 1, 1, 1),  # White color
            size_hint_y=None,
            height=50
        )
        scroll_layout.add_widget(self.relay_label)

        # Label for the threshold counter
        self.threshold_counter = Label(
            text='Željena temperatura: 27 °C',  # Initial threshold value
            font_size='20sp',
            color=(1, 1, 1, 1),  # White color
            size_hint_y=None,
            height=50
        )
        scroll_layout.add_widget(self.threshold_counter)

        # Slider for setting the threshold
        self.threshold_slider = Slider(min=10, max=30, value=27, step=1, size_hint_y=None, height=50)
        self.threshold_slider.bind(value=self.on_slider_value_change)  # Bind value change to update label and send data
        self.threshold_slider.bind(on_touch_up=self.on_slider_touch_up)  # Bind touch release event to send data
        scroll_layout.add_widget(self.threshold_slider)

        # Add scroll layout to scroll view
        scroll_view.add_widget(scroll_layout)
        self.layout.add_widget(scroll_view)

        # Fetch current data initially and set interval for auto-refresh
        Clock.schedule_once(lambda dt: self.fetch_data(), 0)
        Clock.schedule_interval(self.fetch_data, 5)  # Update every 5 seconds

        return self.layout

    def fetch_data(self, dt=None):
        try:
            response = requests.get("http://127.0.0.1:5000/fetch")
            if response.status_code == 200:
                data = response.json()
                print("API Response:", data)  # Debugging output

                # Update based on the new API structure
                current_temp = data.get('current_temp', 'Unknown')  # Fetch current temperature
                relay_status = "On" if data.get('relay_status') else "Off"  # Fetch relay status
                threshold_value = int(self.threshold_slider.value)  # Get current threshold value

                self.temp_label.text = f'Temperatura: {current_temp} °C'
                self.relay_label.text = f'Status: {relay_status}'
                self.threshold_counter.text = f'Željena temperatura: {threshold_value} °C'

                self.error_occurred = False  # Reset error flag if successful
            else:
                self.handle_error(f'Error fetching data: {response.status_code}')
        except requests.exceptions.RequestException as e:
            self.handle_error(f'Error: {str(e)}')

    def on_slider_value_change(self, instance, value):
        """Update the threshold counter label as the slider moves."""
        self.threshold_counter.text = f'Željena temperatura: {int(value)} °C'  # Update label immediately

    def on_slider_touch_up(self, instance, touch):
        """Send new threshold to the API when the user releases the slider."""
        if touch.grab_current is self.threshold_slider:
            new_threshold = int(self.threshold_slider.value)  # Convert to int for sending
            self.set_threshold(new_threshold)  # Send new threshold to the API

    def set_threshold(self, new_threshold):
        try:
            # Format the threshold value to an integer within the acceptable range
            new_threshold = int(new_threshold)

            # Send a GET request to the Flask API with the threshold value in the URL path
            response = requests.get(f"http://127.0.0.1:5000/setThreshold/{new_threshold}")

            # Check the response status code
            if response.status_code == 200:
                self.threshold_counter.text = f'Željena temperatura: {new_threshold} °C'
                #self.temp_label.text = 'Threshold updated successfully!'
                self.error_occurred = False
                Clock.schedule_once(lambda dt: self.reset_temp_label(), 2)
            else:
                self.handle_error(f'Error setting threshold: {response.status_code} - {response.text}')
        except requests.exceptions.RequestException as e:
            self.handle_error(f'Error: {str(e)}')
        except ValueError:
            self.handle_error('Error: Invalid threshold value provided. It must be an integer.')

    def reset_temp_label(self):
        self.temp_label.text = f'Temperatura: -- °C'  # Reset to default

    def handle_error(self, error_message):
        self.temp_label.text = error_message
        self.relay_label.text = ''
        self.error_occurred = True
        Clock.schedule_interval(self.check_connection, 5)  # Check connection every 5 seconds

    def check_connection(self, dt):
        try:
            response = requests.get("http://127.0.0.1:5000/fetch")
            if response.status_code == 200:
                self.fetch_data()  # Try to fetch data again
                Clock.unschedule(self.check_connection)  # Stop checking if successful
            else:
                self.temp_label.text = f'Retrying... (Last error: {response.status_code})'
        except requests.exceptions.RequestException:
            self.temp_label.text = 'Connection still down, retrying...'

if __name__ == '__main__':
    ThermostatApp().run()
