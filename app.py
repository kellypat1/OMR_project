from flask import Flask, request, render_template, redirect, url_for, jsonify, send_file
import os
import subprocess

app = Flask(__name__)

# Set the folders where uploaded files will be stored
UPLOAD_FOLDER_STUDENTS = 'students'
UPLOAD_FOLDER_ANSWERS = 'answers'

app.config['UPLOAD_FOLDER_STUDENTS'] = UPLOAD_FOLDER_STUDENTS
app.config['UPLOAD_FOLDER_ANSWERS'] = UPLOAD_FOLDER_ANSWERS

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/upload', methods=['POST'])
def upload():
    if 'students[]' in request.files:
        students_files = request.files.getlist('students[]')

        for file in students_files:
            if file.filename == '':
                continue
            filename = os.path.join(app.config['UPLOAD_FOLDER_STUDENTS'], file.filename)
            file.save(filename)

    if 'answers[]' in request.files:
        answers_files = request.files.getlist('answers[]')

        for file in answers_files:
            if file.filename == '':
                continue
            filename = os.path.join(app.config['UPLOAD_FOLDER_ANSWERS'], file.filename)
            file.save(filename)

    # Redirect back to the main platform with a success message as a query parameter
    return redirect(url_for('index', success='Files uploaded successfully'))

@app.route('/reset')
def reset():
    try:
        exclude_file = '.ignorethatfile'
        # Remove all files from the "students" folder
        students_folder = app.config['UPLOAD_FOLDER_STUDENTS']
        for filename in os.listdir(students_folder):
            if filename != exclude_file:
                file_path = os.path.join(students_folder, filename)
                if os.path.isfile(file_path):
                    os.remove(file_path)

        # Remove all files from the "answers" folder
        answers_folder = app.config['UPLOAD_FOLDER_ANSWERS']

        for filename in os.listdir(answers_folder):
            if filename != exclude_file:
                file_path = os.path.join(answers_folder, filename)
                if os.path.isfile(file_path):
                    os.remove(file_path)

        return redirect(url_for('index', success='All files deleted successfully'))
    except Exception as e:
        return str(e)

@app.route('/results', methods=['POST'])
def run_results():
    try:
        exam_name = request.form.get('examName')
        exam_date = request.form.get('examDate')
        
        # Create the text file using subprocess
        #subprocess.run(['./BUBBLESHEET', os.path.join("results", f"{exam_name}_{exam_date}.txt")], check=True)
        subprocess.run(['./BUBBLESHEET'], check=True)
        # Define the path for the text file
        
        file_to_find = "exam_sheet.xlsx"
        new_file_name = f"{exam_name}_{exam_date}.xlsx"
        current_directory = os.getcwd()
        files = os.listdir(current_directory)

        if file_to_find in files:
            old_file_path = os.path.join(current_directory,file_to_find)
            new_file_path = os.path.join(current_directory,new_file_name)
            os.rename(old_file_path,new_file_path)
            print("file renamed to: ",new_file_name)
        else:
            print("file not found")
        txt_file_path = os.path.join(f"{exam_name}_{exam_date}.xlsx")
        
        # Create a response with the text file for download
        return send_file(
            txt_file_path,
            as_attachment=True,
            download_name=f"{exam_name}_{exam_date}.xlsx",
            mimetype="text/plain",
        )

    except Exception as e:
        return jsonify({"message": f"An error occurred: {str(e)}"})

if __name__ == '__main__':
    app.run(debug=True)