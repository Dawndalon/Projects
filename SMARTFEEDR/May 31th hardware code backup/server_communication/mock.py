from flask import Flask, request, jsonify
import os
import json
from datetime import datetime

app = Flask(__name__)

# Directory where the files will be written and saved
UPLOAD_FOLDER_TEXTS = '/home/cse123team4/cse123/test/storage/text'
UPLOAD_FOLDER_IMAGES = '/home/cse123team4/cse123/test/storage/imagines'
COMMANDS_FOLDER = '/home/cse123team4/cse123/test/ins'
COMMANDS_FILE = 'instructions.json'
# DATA_JSON_FILE = os.path.join(UPLOAD_FOLDER_TEXTS, 'data.json')
TEXTS_FILE = 'received_texts.txt'

# Ensure the necessary folders exist
if not os.path.exists(UPLOAD_FOLDER_TEXTS):
    os.makedirs(UPLOAD_FOLDER_TEXTS)
if not os.path.exists(COMMANDS_FOLDER):
    os.makedirs(COMMANDS_FOLDER)
if not os.path.exists(UPLOAD_FOLDER_IMAGES):
    os.makedirs(UPLOAD_FOLDER_IMAGES)

            
@app.route('/api/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({"error": "No file part"}), 400
    uploaded_file = request.files['file']
    if uploaded_file.filename == '':
        return jsonify({"error": "No selected file"}), 400

    filename = uploaded_file.filename.lower()
    if filename.endswith('.txt'):
        file_path = os.path.join(UPLOAD_FOLDER_TEXTS, TEXTS_FILE)
        try:
            # Receive a single line from the client
            file_content = uploaded_file.read().decode('utf-8').strip() + '\n'
            if not file_content.strip():
                return jsonify({"error": "Empty content received"}), 400
            # Append the received text to the file
            with open(file_path, 'a', encoding='utf-8') as f:
                f.write(file_content)
        except UnicodeDecodeError:
            return jsonify({"error": "Invalid text encoding"}), 400

    elif filename.endswith(('.png', '.jpg', '.jpeg', '.gif', '.svg', '.bmp', '.tiff')):
        file_path = os.path.join(UPLOAD_FOLDER_IMAGES, filename)
        try:
            with open(file_path, 'wb') as f:
                f.write(uploaded_file.read())
        except Exception as e:
            return jsonify({"error": f"Failed to save image: {str(e)}"}), 500

    else:
        return jsonify({"error": "Unsupported file type"}), 400

    print(f"Received file: {filename}, processed successfully.")
    return jsonify({"status": "success", "message": f"File {filename} received and processed successfully"})


@app.route('/api/commands', methods=['GET', 'POST'])
def handle_commands():
    command_path = os.path.join(COMMANDS_FOLDER, COMMANDS_FILE)
    if request.method == 'POST':
        data = request.json
        if not data or 'command' not in data:
            return jsonify({"error": "No command provided"}), 400
        command = data['command']
        timestamp = datetime.now().isoformat()
        try:
            with open(command_path, "r") as file:
                commands = json.load(file)
        except FileNotFoundError:
            commands = []
        commands.append({"timestamp": timestamp, "command": command})
        with open(command_path, "w") as file:
            json.dump(commands, file)
        return jsonify({"message": "Command received and written"}), 200
    elif request.method == 'GET':
        last_timestamp = request.args.get('last_timestamp')
        try:
            with open(command_path, "r") as file:
                commands = json.load(file)
            if last_timestamp:
                filtered_commands = [cmd for cmd in commands if cmd['timestamp'] > last_timestamp]
                return jsonify({"commands": filtered_commands}), 200
            return jsonify({"commands": commands}), 200
        except FileNotFoundError:
            commands = []  # Create an empty list if file not found
            with open(command_path, "w") as file:
                json.dump(commands, file)  # Create the file
            return jsonify({"commands": commands}), 200


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001, debug=True)