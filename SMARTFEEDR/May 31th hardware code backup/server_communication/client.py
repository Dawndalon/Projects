import os
import threading
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from datetime import datetime, timedelta
from dotenv import load_dotenv
import requests
import time
import json

COMMANDS_FILE = 'instructions.txt'
UPLOAD_FOLDER_TEXTS = '/home/cse123team4/cse123'
WATCHED_FOLDER_PICTURES = '/home/cse123team4/cse123/images'
WATCHED_FOLDER_TEXTS = '/home/cse123team4/cse123/data'

# TARGET_SERVER_URL = 'http://localhost:5001/api/upload'
# command_url = 'http://localhost:5001/api/commands'
# PIC_SERVER_URL = 'http://localhost:5001/api/upload_picture'
# DEVICE_STATUS_URL = 'http://localhost:5001/api/device_status'

TARGET_SERVER_URL = 'https://cse123petfeeder.com/api/upload'
command_url = 'https://cse123petfeeder.com/api/commands'
PIC_SERVER_URL = 'https://cse123petfeeder.com/api/upload_picture'
DEVICE_STATUS_URL = 'https://cse123petfeeder.com/api/device_status'

# TARGET_SERVER_URL = 'http://172.20.10.6:8080/api/upload'
# command_url = 'http://172.20.10.6:8080/api/commands'
# PIC_SERVER_URL = 'http://172.20.10.6:8080/api/upload_picture'
# DEVICE_STATUS_URL = 'http://172.20.10.6:8080/api/device_status'

load_dotenv()

bearer_token = os.environ.get("BEARER_TOKEN")
if bearer_token is None:
    raise ValueError("Bearer token is not set in the environment variable")

# Ensure the necessary folders exist
if not os.path.exists(UPLOAD_FOLDER_TEXTS):
    os.makedirs(UPLOAD_FOLDER_TEXTS)
if not os.path.exists(WATCHED_FOLDER_PICTURES):
    os.makedirs(WATCHED_FOLDER_PICTURES)
if not os.path.exists(WATCHED_FOLDER_TEXTS):
    os.makedirs(WATCHED_FOLDER_TEXTS)


# Initialize last_sent_times dictionary
last_sent_times = {
    'FoodData.txt': datetime.min,
    'WaterData.txt': datetime.min
}

def send_file_to_server(file_path):
    """Send text files line by line as plain text to the server, with appropriate handling for binary files."""
    global last_sent_times
    now = datetime.now()

    file_name = os.path.basename(file_path)
    if file_name not in last_sent_times:
        print(f"File {file_name} is not being monitored.")
        return

    bearer_token = os.environ.get("BEARER_TOKEN")
    if not bearer_token:
        print("Bearer token is not set in environment variable")
        return
    headers = {
        'Authorization': f'Bearer {bearer_token}',
        'Content-Type': 'text/plain'  # Set content type to plain text for sending lines
    }

    is_text_file = file_path.endswith('.txt')

    # Check cooldown for text files
    if is_text_file and now - last_sent_times[file_name] < timedelta(seconds=10):
        print("Cooldown period active. Skipping file sending.")
        return

    try:
        if is_text_file:
            # Open the file in text mode to read lines individually
            with open(file_path, 'r', encoding='utf-8') as file:
                lines = file.readlines()

            for line in lines:
                line = line.strip()
                if not line:
                    continue

                # Debug: Print the line being sent
                print(f"Sending line: {line}")

                # Send each line individually as plain text
                response = requests.post(TARGET_SERVER_URL, data=line.encode('utf-8'), headers=headers)
                print(f"Line sent to server from {file_path}, Status Code: {response.status_code}")

                # Check if the response is successful
                if response.status_code != 200:
                    raise Exception(f"Failed to send line: {line}")

            # Clear the file after sending all lines
            with open(file_path, 'w') as file:
                file.truncate(0)
            last_sent_times[file_name] = now

    except requests.exceptions.RequestException as e:
        print(f"Network error while sending data to server: {e}")
    except Exception as e:
        print(f"Failed to send {file_path} to server: {e}")


def send_picture_to_server(file_path):
    headers = {
        'Authorization': f'Bearer {bearer_token}'
    }

    try:
        with open(file_path, 'rb') as file:
            files = {'file': (os.path.basename(file_path), file.read())}

        response = requests.post(PIC_SERVER_URL, files=files, headers=headers)
        print(f"Binary file sent to server from {file_path}, Status Code: {response.status_code}")

        if response.status_code != 200:
            raise Exception(f"Failed to send binary file: {file_path}")


    except requests.exceptions.RequestException as e:
        print(f"Network error while sending data to server: {e}")
    except Exception as e:
        print(f"Failed to send {file_path} to server: {e}")


class FileEventHandler(FileSystemEventHandler):
    # def on_modified(self, event):
    #     if not event.is_directory and 'data.txt' == os.path.basename(event.src_path):
    #         print(f"Modification detected in: {event.src_path}")
    #         send_file_to_server(event.src_path)
    def on_modified(self, event):
        monitored_files = ['FoodData.txt', 'WaterData.txt']
        if not event.is_directory and os.path.basename(event.src_path) in monitored_files:
            print(f"Modification detected in: {event.src_path}")
            send_file_to_server(event.src_path)

    def on_created(self, event):
        if not event.is_directory and event.src_path.endswith(('.png', '.jpg', '.jpeg', '.gif')):
            print(f"New picture detected: {event.src_path}")
            send_picture_to_server(event.src_path)

def start_watcher():
    observer = Observer()
    observer.schedule(FileEventHandler(), WATCHED_FOLDER_PICTURES, recursive=True)
    observer.schedule(FileEventHandler(), WATCHED_FOLDER_TEXTS, recursive=True)
    observer.start()
    try:
        observer.join()
    except Exception as e:
        print(f"Error starting observer: {e}")

full_command_file_path = os.path.join(UPLOAD_FOLDER_TEXTS, COMMANDS_FILE)


last_timestamp = None  # Keep track of the last fetched command's timestamp

def fetch_and_write_commands():
    global last_timestamp
    while True:
        # fetch the bearer token from environment variables in bashrc
        # THIS IS FOR SECURE CONNECTIN BETWEEN PI AND SERVER
        bearer_token = os.environ.get("BEARER_TOKEN")
        if not bearer_token:
            print("Bearer token is not set in environment variables")
            continue
        headers = {'Authorization': f'Bearer {bearer_token}'}
        params = {'last_timestamp': last_timestamp} if last_timestamp else {}
        try:
            response = requests.get(command_url, params=params, headers=headers)
            if response.status_code == 200:
                commands_data = response.json().get('commands', [])
                if commands_data:
                    full_path = os.path.join(UPLOAD_FOLDER_TEXTS, COMMANDS_FILE)
                    with open(full_path, 'a') as file:  # Open in append mode to add new commands
                        for command in commands_data:
                            file.write(f"{command['command']}\n")
                    last_timestamp = commands_data[-1]['timestamp']  # Update the last timestamp
                    print("Commands updated locally.")
                else:
                    print("No new commands to update.")
            else:
                print("Failed to fetch commands:", response.status_code, response.text)
            time.sleep(10)  # Sleep some time before fetching again
        except requests.exceptions.RequestException as e:
            print(f"Error fetching commands: {e}")


def update_device_status(online_status):
    headers = {'Content-Type': 'application/json'}
    data = {'online': online_status}
    try:
        response = requests.post(DEVICE_STATUS_URL, json=data, headers=headers)
        print(response.json())
    except requests.exceptions.RequestException as e:
        print(f"Error updating device status: {e}")

if __name__ == "__main__":
    # Update device status to online
    update_device_status(True)

    # Start the watcher thread
    watcher_thread = threading.Thread(target=start_watcher)
    # Start the command fetch thread
    command_fetch_thread = threading.Thread(target=fetch_and_write_commands)

    watcher_thread.start()
    command_fetch_thread.start()

    try:
        while True:
            time.sleep(60)  # Check status every 60 seconds
    except KeyboardInterrupt:
        print("Shutting down...")

    # Update device status to offline before exiting
    update_device_status(False)

    watcher_thread.join()  # Optionally wait for the watcher thread to finish
    command_fetch_thread.join()  # Optionally wait for the command fetch thread to finish
