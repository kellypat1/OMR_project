#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <iostream>
#include <experimental/filesystem>
#include <OpenXLSX.hpp>

#include "transform.hpp"

using namespace cv;
using namespace OpenXLSX;
using namespace std;
namespace fs = std::experimental::filesystem;

const int NoOfChoice = 5;
const int NoOfAem = 10;
const int NoOfQuestion = 20;

std::map<int, int> standardAnswer;
std::map<int, int> testerAnswer;
string testerAem;
XLDocument doc;

static string to_string_answer(int answer)
{
    if (answer == 0)
        return "a";
    else if (answer == 1)
        return "b";
    else if (answer == 2)
        return "c";
    else if (answer == 3)
        return "d";
    else if (answer == 4)
        return "e";
    else
        return "-";
}

static void AppendMyData()
{
    static int counter = 2;
    auto wks = doc.workbook().worksheet("Sheet1");
    std::map<int, int>::const_iterator itStandardAnswer;
    std::map<int, int>::const_iterator itTesterAnswer;

    itStandardAnswer = standardAnswer.begin();
    itTesterAnswer = testerAnswer.begin();

    int currentQuestion = 0;

    wks.cell(counter + 1, 1).value() = testerAem;
    while (itStandardAnswer != standardAnswer.end())
    {
        string answer;

        answer = to_string_answer(itTesterAnswer->second);
        if (itTesterAnswer->second == 100) {
            answer.append("/to doubt");
        } 
        if (itStandardAnswer->second != itTesterAnswer->second && itTesterAnswer->second != 100)
        {
            answer.append("/wrong");
        }
        ++currentQuestion;
        ++itTesterAnswer;
        ++itStandardAnswer;
        wks.cell(counter + 1, currentQuestion + 1).value() = answer;
    }
    counter += 1;
}

static void CreationOfExcel()
{
    std::string path = "exam_sheet.xlsx";
    std::map<int, int>::const_iterator itStandardAnswer;

    doc.create(path);
    auto wks = doc.workbook().worksheet("Sheet1");

    wks.cell(1, 1).value() = "AEM";
    for (int i = 2; i < 22; i++)
    {
        string question = "Question ";
        question.append(to_string(i - 1));
        wks.cell(1, i).value() = question;
    }
    itStandardAnswer = standardAnswer.begin();
    int currentQuestion = 0;

    wks.cell(2, 1).value() = "Right Answers";
    while (itStandardAnswer != standardAnswer.end())
    {
        string answer;

        answer = to_string_answer(itStandardAnswer->second);

        ++itStandardAnswer;
        ++currentQuestion;

        wks.cell(2, currentQuestion + 1).value() = answer;
    }
}

static std::map<int, int> DetermineMarkedAnswer(Mat thresh, std::vector<Vec4i> hierarchy, std::vector<std::vector<Point>> *Counter, int is_answer, int wrong_calculation)
{
    std::map<int, int> Answer;
    int choice = (is_answer) ? NoOfChoice : NoOfAem;
    int size = (is_answer) ? 100 : 50;

    for (int i = 0; i < size;)
    {
        int maxPixel = 0, maxPixel1 = 0;
        int answerKey = 0;
        int count_answers = 1;
        Mat mask;
        for (int j = 0; j < choice; ++i, ++j)
        {
            if (wrong_calculation)
            {
                answerKey = -1;
                continue;
            }
            mask = Mat::zeros(thresh.size(), CV_8U);
            drawContours(mask, *Counter, i, 255, FILLED, 8, hierarchy, 0, Point());
           
            bitwise_and(mask, thresh, mask);
            
            if (countNonZero(mask) > maxPixel)
            {
                maxPixel = countNonZero(mask);
                answerKey = j;
            }
        }
        if (maxPixel <100) answerKey ==100;
        Answer.insert(std::make_pair(i / choice - 1, answerKey));
    }
    return Answer;
}

static int DetermineOfAnswer(int is_answer, int w, int h)
{
    double ar = (double)w / h;
    
    if (is_answer)
    {
        if (w >= 8 && h >= 8)
        {
            if (w <= 40 && h <= 40 /*&& ar<=1.4 && ar >=0.6*/)
            {
                return 1;
            }
        }
    }    
    else
    {
        if (w >= 8 && h >= 8)
        {
            if (w <= 40 && h <= 40)
            {
                return 1;
            }
        }
    }
    return 0;
}

static std::map<int, int> ProcessToDetermineMarkedAnswer(Mat wrapped, Mat paper, std::vector<std::vector<Point>> contours, std::vector<std::vector<Point>> *Counter, int is_answer)
{
    Mat thresh, kernel, eroded, edges;
    std::vector<Vec4i> hierarchy, lines;
    int wrong_calculation = 0;


    // Step 3: Extract the sets of bubbles (questionCnt)

    adaptiveThreshold(wrapped, thresh, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 11, 10);

    findContours(thresh, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_NONE, Point(0, 0));
   
    std::vector<std::vector<Point>> contours_poly(contours.size());
    std::vector<Rect> boundRect(contours.size());
    
    // Find document countour
    for (int i = 0; i < contours.size(); i++)
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 0.01, true);
        int w = boundingRect(Mat(contours[i])).width;
        int h = boundingRect(Mat(contours[i])).height;
        double ar = (double)w / h;

        if (hierarchy[i][3] == -1)
        {
            if (DetermineOfAnswer(is_answer, w, h))
            {
                Counter->push_back(contours_poly[i]);
            }
        }
    }
    
      if (is_answer)
      {
          if (Counter->size() != 100)
          {
              wrong_calculation = 1;
          }
      }
      else
      {
          if (Counter->size() != 50)
          {
              wrong_calculation = 1;
          }
      }
    
    // Step 4: Sort the questions/bubbles into rows.
 if (!wrong_calculation)
    {
        int k = 0;
        sort_contour(*Counter, 0, (int)Counter->size(), std::string("top-to-bottom"));
        if (is_answer)
        {
            for (int i = 0; i < Counter->size(); i = i + NoOfChoice)
            {
                k++;
                sort_contour(*Counter, i, i + 5, std::string("left-to-right"));
            }
        }
        else
        {
            for (int i = 0; i < Counter->size(); i = i + NoOfAem)
            {
                k++;
                sort_contour(*Counter, i, i + NoOfAem, std::string("left-to-right"));
            }
        }
    }
    return DetermineMarkedAnswer(thresh, hierarchy, Counter, is_answer, wrong_calculation);
}

static vector<Point> FindAnswersContour(std::vector<std::vector<Point>> contours)
{
    std::vector<Point> approxCurve, docCnt;

    for (const auto &element : contours)
    {

        int w = boundingRect(Mat(element)).width;
        int h = boundingRect(Mat(element)).height;
        double perimeter = arcLength(element, true);

        approxPolyDP(element, approxCurve, 0.07 * perimeter, true);

        if (approxCurve.size() == 4)
        {

            docCnt = approxCurve;
            break;
        }
    }
    return docCnt;
}

static vector<Point> FindPageContour(std::vector<std::vector<Point>> contours)
{
    std::vector<Point> approxCurve, docCnt;

    for (const auto &element : contours)
    {

        int w = boundingRect(Mat(element)).width;
        int h = boundingRect(Mat(element)).height;
        double perimeter = arcLength(element, true);
        approxPolyDP(element, approxCurve, 0.05 * perimeter, true);

        if (approxCurve.size() == 4 || approxCurve.size() == 2)
        {
            docCnt = approxCurve;
            break;
        }
    }
    return docCnt;
}

static vector<Point> FindAemContour(std::vector<std::vector<Point>> contours)
{
    int counter = 0;
    std::vector<Point> approxCurve, docCnt;

    for (const auto &element : contours)
    {
        int w = boundingRect(Mat(element)).width;
        int h = boundingRect(Mat(element)).height;
        double perimeter = arcLength(element, true);
        approxPolyDP(element, approxCurve, 0.07 * perimeter, true);
        if (approxCurve.size() == 4)
        {
            counter++;
            if (counter == 2)
            {
                docCnt = approxCurve;
                break;
            }
        }
    }
    return docCnt;
}

static int ComputeAnswerAndAemMap(int is_tester)
{
    std::string path;
    if (!is_tester)
    {
        path = "answers";
    }
    else
    {
        path = "students";
    }
    for (const auto &entry : fs::directory_iterator(path))
    {
        Mat image, gray, blurred, edge, thresh1, edge2;
        if (is_tester)
            testerAnswer.clear();
        image = imread(entry.path(), IMREAD_COLOR);

        if (image.empty())
        {
            std::cout << "Failed to read file\n";
            return 0;
        }
        cvtColor(image, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, blurred, Size(7, 7), 0);
        threshold(blurred, edge, 127, 255, THRESH_BINARY_INV | THRESH_OTSU);
        resize(edge, edge, Size(750, 1000), INTER_LANCZOS4);
        resize(image, image, Size(750, 1000), INTER_LANCZOS4);
        resize(gray, gray, Size(750, 1000), INTER_LANCZOS4);

        Mat paper, wrapped, thresh, edge_wrapped, image_wrapped, gray_wrapped;
        std::vector<std::vector<Point>> contours_outside;
        std::vector<Point> docCnt;
        std::vector<std::vector<Point>> contours;
        std::vector<Vec4i> hierarchy;
        std::vector<std::vector<Point>> questionCnt;
        std::vector<std::vector<Point>> aemCnt;

        /// Find contours
        findContours(edge, contours_outside, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));
        std::sort(contours_outside.begin(), contours_outside.end(), compareContourAreas);
        drawContours(image, contours_outside, -1, Scalar(0, 255, 0), 2, 8, hierarchy, 0, Point());

        // Find document countour
        docCnt = FindAnswersContour(contours_outside);

        // Step 2: Apply a perspective transform to extract the top-down, birds-eye-view of the exam

        four_point_transform(edge, edge_wrapped, docCnt);

        four_point_transform(image, image_wrapped, docCnt);

        four_point_transform(gray, gray_wrapped, docCnt);

        if (is_tester)
        {
            testerAnswer = ProcessToDetermineMarkedAnswer(gray_wrapped, image_wrapped, contours_outside, &questionCnt, 1);
        }
        else
        {
            standardAnswer = ProcessToDetermineMarkedAnswer(gray_wrapped, image_wrapped, contours_outside, &questionCnt, 1);
        }
        if (is_tester)
        {
            Mat image_wrapped, gray_wrapped;
            testerAem.clear();
            std::sort(contours_outside.begin(), contours_outside.end(), compareContourAreas);

            docCnt = FindAemContour(contours_outside);
            std::map<int, int> data;

            four_point_transform(image, image_wrapped, docCnt);

            four_point_transform(gray, gray_wrapped, docCnt);

            data = ProcessToDetermineMarkedAnswer(gray_wrapped, image_wrapped, contours_outside, &aemCnt, 0);

            for (int i = 0; i < 5; i++)
            {
                if (data.at(i) == -1)
                {
                    testerAem.append("-");
                    continue;
                }
                testerAem.append(to_string(data.at(i)));
            }
            if (!doc)
                CreationOfExcel();
            AppendMyData();
        }
    }
    if (doc)
    {
        doc.save();
        doc.close();
    }
    return 1;
} /*ComputeStandardAnswerMap*/

int main(int argc, char **argv)
{
    if (!ComputeAnswerAndAemMap(0))
        return 0;
    ComputeAnswerAndAemMap(1);

    return 0;
}