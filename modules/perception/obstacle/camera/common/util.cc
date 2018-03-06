/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "modules/perception/obstacle/camera/common/util.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <iomanip>

#include "gflags/gflags.h"

namespace apollo {
namespace perception {

DEFINE_string(onsemi_obstacle_extrinsics,
              "./conf/params/onsemi_obstacle_extrinsics.yaml",
              "onsemi_obstacle_extrinsics name");
DEFINE_string(onsemi_obstacle_intrinsics,
              "./conf/params/onsemi_obstacle_intrinsics.yaml",
              "onsemi_obstacle_intrinsics name");
DEFINE_bool(onboard_undistortion, false, "do undistortion");

std::vector<cv::Scalar> color_table = {
    cv::Scalar(0, 0, 0),       cv::Scalar(128, 0, 0),
    cv::Scalar(255, 0, 0),     cv::Scalar(0, 128, 0),
    cv::Scalar(128, 128, 0),   cv::Scalar(255, 128, 0),
    cv::Scalar(0, 255, 0),     cv::Scalar(128, 255, 0),
    cv::Scalar(255, 255, 0),   cv::Scalar(0, 0, 128),
    cv::Scalar(128, 0, 128),   cv::Scalar(255, 0, 128),
    cv::Scalar(0, 128, 128),   cv::Scalar(128, 128, 128),
    cv::Scalar(255, 128, 128), cv::Scalar(0, 255, 128),
    cv::Scalar(128, 255, 128), cv::Scalar(255, 255, 128),
    cv::Scalar(0, 0, 255),     cv::Scalar(128, 0, 255),
    cv::Scalar(255, 0, 255),   cv::Scalar(0, 128, 255),
    cv::Scalar(128, 128, 255), cv::Scalar(255, 128, 255),
    cv::Scalar(0, 255, 255),   cv::Scalar(128, 255, 255),
    cv::Scalar(255, 255, 255),
};

bool load_visual_object_form_file(
    const std::string &file_name,
    std::vector<VisualObjectPtr> *visual_objects) {
  FILE *fp = fopen(file_name.c_str(), "r");
  if (!fp) {
    std::cout << "load file: " << file_name << " error!" << std::endl;
    return false;
  }

  while (!feof(fp)) {
    VisualObjectPtr obj(new VisualObject());
    std::fill(
        obj->type_probs.begin(),
        obj->type_probs.begin() + static_cast<int>(ObjectType::MAX_OBJECT_TYPE),
        0);

    double trash = 0.0;
    char type[255];
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    int ret =
        fscanf(fp,
               "%s %lf %lf %f %lf %lf %lf %lf %f %f %f %f %f %f %f %f "
               "%lf %lf",
               type, &trash, &trash, &obj->alpha, &x1, &y1, &x2, &y2,
               &obj->height, &obj->width, &obj->length, &obj->center.x(),
               &obj->center.y(), &obj->center.z(), &obj->theta, &obj->score,
               &obj->truncated_vertical, &obj->truncated_horizontal);
    obj->upper_left[0] = x1 > 0 ? x1 : 0;
    obj->upper_left[1] = y1 > 0 ? y1 : 0;
    obj->lower_right[0] = x2 < 1920 ? x2 : 1920;
    obj->lower_right[1] = y2 < 1080 ? y2 : 1080;
    obj->type = get_object_type(std::string(type));
    obj->type_probs[static_cast<int>(obj->type)] =
        static_cast<float>(obj->score);

    if (ret >= 15) {
      visual_objects->push_back(obj);
    }
  }
  fclose(fp);
  return true;
}

bool write_visual_object_to_file(const std::string &file_name,
                                 std::vector<VisualObjectPtr> *visual_objects) {
  FILE *fp = fopen(file_name.c_str(), "w");
  if (!fp) {
    std::cout << "write file: " << file_name << " error!" << std::endl;
    return false;
  }

  for (auto &obj : *visual_objects) {
    std::string type = get_type_text(obj->type);

    fprintf(fp,
            "%s %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f "
            "%.2f %.2f %.2f\n",
            type.c_str(), 0.0, 0.0, obj->alpha, obj->upper_left[0],
            obj->upper_left[1], obj->lower_right[0], obj->lower_right[1],
            obj->height, obj->width, obj->length, obj->center.x(),
            obj->center.y(), obj->center.z(), obj->theta, obj->score);
  }

  fclose(fp);
  return true;
}

bool load_gt_form_file(const std::string &gt_path,
                       std::vector<VisualObjectPtr> *visual_objects) {
  std::ifstream gt_file(gt_path);

  std::string line;
  int detected_id = 0;
  while (std::getline(gt_file, line)) {
    std::istringstream iss(line);

    std::vector<std::string> tokens;
    std::copy(std::istream_iterator<std::string>(iss),
              std::istream_iterator<std::string>(), back_inserter(tokens));

    // TODO(All) Decide where to Eliminate -99 2D only cases, since they don't
    // have 3d (2409 test)
    if (tokens.size() == 16 && tokens[3].compare("-99") != 0) {
      VisualObjectPtr obj(new VisualObject());

      // TODO(All) Why the 2409 test data ground truth has exchanged alpha and
      // theta
      // position?

      obj->type = get_object_type(tokens[0]);

      //            obj->alpha = std::stod(tokens[3]);
      obj->alpha = std::stod(tokens[14]);

      obj->upper_left[0] = std::stod(tokens[4]);
      obj->upper_left[1] = std::stod(tokens[5]);
      obj->lower_right[0] = std::stod(tokens[6]);
      obj->lower_right[1] = std::stod(tokens[7]);
      obj->height = std::stod(tokens[8]);
      obj->width = std::stod(tokens[9]);
      obj->length = std::stod(tokens[10]);
      obj->center.x() = std::stod(tokens[11]);
      obj->center.y() = std::stod(tokens[12]);
      obj->center.z() = std::stod(tokens[13]);

      //            obj->theta = std::stod(tokens[14]);
      obj->theta = std::stod(tokens[3]);

      obj->score = std::stod(tokens[15]);
      obj->id = detected_id++;

      // Set binary truncation estimation from gt box position
      // (Only possible to do this with this kind of gt, not detection)
      // (2 pixels to frame boundaries)
      if (obj->upper_left[0] <= 2.0 || obj->lower_right[0] >= 1918.0) {
        obj->truncated_horizontal = 0.5;
      } else {
        obj->truncated_horizontal = 0.0;
      }

      if (obj->upper_left[1] <= 2.0 || obj->lower_right[1] >= 1078.0) {
        obj->truncated_vertical = 0.5;
      } else {
        obj->truncated_vertical = 0.0;
      }

      visual_objects->push_back(obj);
    }
  }
  return true;
}

void draw_visual_objects(const std::vector<VisualObjectPtr> &visual_objects,
                         cv::Mat *img) {
  for (const auto &obj : visual_objects) {
// 3D BBox
#if 0
        if (obj->pts8.size() >= 17) {
            cv::Point pixels[9];
            for (size_t i = 0; i < 9; ++i) {
                pixels[i] = cv::Point(static_cast<int>(obj->pts8[i*2]),
                                      static_cast<int>(obj->pts8[i*2 + 1]));
            }

            cv::line(*img, pixels[4], pixels[5], COLOR_GREEN, 1);
            cv::line(*img, pixels[5], pixels[6], COLOR_YELLOW, 1);
            cv::line(*img, pixels[6], pixels[7], COLOR_BLUE, 1);
            cv::line(*img, pixels[7], pixels[4], COLOR_YELLOW, 1);

            cv::line(*img, pixels[0], pixels[1], COLOR_GREEN, 1);
            cv::line(*img, pixels[1], pixels[2], COLOR_YELLOW, 1);
            cv::line(*img, pixels[2], pixels[3], COLOR_BLUE, 1);
            cv::line(*img, pixels[3], pixels[0], COLOR_YELLOW, 1);

            cv::line(*img, pixels[0], pixels[4], COLOR_GREEN, 1);
            cv::line(*img, pixels[1], pixels[5], COLOR_GREEN, 1);
            cv::line(*img, pixels[2], pixels[6], COLOR_BLUE, 1);
            cv::line(*img, pixels[3], pixels[7], COLOR_BLUE, 1);

            // 3D center
            cv::circle(*img, pixels[8], 3, COLOR_RED, 3);
        }
#endif

    // 2D BBox
    int x1 = static_cast<int>(obj->upper_left[0]);
    int y1 = static_cast<int>(obj->upper_left[1]);
    int x2 = static_cast<int>(obj->lower_right[0]);
    int y2 = static_cast<int>(obj->lower_right[1]);

    cv::Point bbox[4];
    bbox[0].x = x1;
    bbox[0].y = y1;
    bbox[1].x = x2;
    bbox[1].y = y1;
    bbox[2].x = x2;
    bbox[2].y = y2;
    bbox[3].x = x1;
    bbox[3].y = y2;

    cv::line(*img, bbox[0], bbox[1], COLOR_WHITE, 1);
    cv::line(*img, bbox[1], bbox[2], COLOR_WHITE, 1);
    cv::line(*img, bbox[2], bbox[3], COLOR_WHITE, 1);
    cv::line(*img, bbox[3], bbox[0], COLOR_WHITE, 1);

    //  Text
    std::stringstream stream;
    stream.str("");
    stream.clear();
    stream << std::fixed << std::setprecision(2)
           << std::sqrt(obj->center.x() * obj->center.x() +
                        obj->center.y() * obj->center.y() +
                        obj->center.z() * obj->center.z());
    stream << " m, alpha:";
    stream << std::fixed << std::setprecision(2) << obj->alpha * 180.0 / M_PI;
    stream << " deg, theta:";
    stream << std::fixed << std::setprecision(2) << obj->theta * 180.0 / M_PI;
    stream << " deg, D:";
    stream << obj->id;
    cv::putText(*img, stream.str(), cv::Point(x1, y1 - 5),
                cv::FONT_HERSHEY_PLAIN, 0.8, COLOR_WHITE);
  }
}

void draw_gt_objects_text(const std::vector<VisualObjectPtr> &visual_objects,
                          cv::Mat *img) {
  for (const auto &obj : visual_objects) {
    // 2D BBox
    int x1 = static_cast<int>(obj->upper_left[0]);
    // int y1 = static_cast<int>(obj->upper_left[1]);
    // int x2 = static_cast<int>(obj->lower_right[0]);
    int y2 = static_cast<int>(obj->lower_right[1]);

    //  Text
    std::stringstream stream;
    stream.str("");
    stream.clear();
    stream << std::fixed << std::setprecision(2)
           << std::sqrt(obj->center.x() * obj->center.x() +
                        obj->center.y() * obj->center.y() +
                        obj->center.z() * obj->center.z());
    stream << " m, alpha:";
    stream << std::fixed << std::setprecision(2) << obj->alpha * 180.0 / M_PI;
    stream << " deg, theta:";
    stream << std::fixed << std::setprecision(2) << obj->theta * 180.0 / M_PI;
    stream << " deg, D:";
    stream << obj->id;
    cv::putText(*img, stream.str(), cv::Point(x1, y2 + 10),
                cv::FONT_HERSHEY_PLAIN, 0.8, COLOR_BLACK);
  }
}

std::string get_type_text(ObjectType type) {
  if (type == ObjectType::VEHICLE) {
    return "car";
  }
  if (type == ObjectType::PEDESTRIAN) {
    return "pedestrian";
  }
  if (type == ObjectType::BICYCLE) {
    return "bicycle";
  }

  return "unknown";
}

ObjectType get_object_type(const std::string &type) {
  std::string temp_type = type;
  std::transform(temp_type.begin(), temp_type.end(), temp_type.begin(),
                 (int (*)(int))std::tolower);
  if (temp_type == "unknown") {
    return ObjectType::UNKNOWN;
  } else if (temp_type == "unknown_movable") {
    return ObjectType::UNKNOWN_MOVABLE;
  } else if (temp_type == "unknown_unmovable") {
    return ObjectType::UNKNOWN_UNMOVABLE;
  } else if (temp_type == "pedestrian") {
    return ObjectType::PEDESTRIAN;
  } else if (temp_type == "bicycle") {
    return ObjectType::BICYCLE;
  } else if (temp_type == "vehicle") {
    return ObjectType::VEHICLE;
  } else if (temp_type == "bus") {
    return ObjectType::VEHICLE;
    // compatible with KITTI output - BEGIN
  } else if (temp_type == "car") {
    return ObjectType::VEHICLE;
  } else if (temp_type == "cyclist") {
    return ObjectType::BICYCLE;
  } else if (temp_type == "dontcare") {
    return ObjectType::UNKNOWN;
  } else if (temp_type == "misc") {
    return ObjectType::UNKNOWN;
  } else if (temp_type == "person_sitting") {
    return ObjectType::PEDESTRIAN;
  } else if (temp_type == "tram") {
    return ObjectType::VEHICLE;
  } else if (temp_type == "truck") {
    return ObjectType::VEHICLE;
  } else if (temp_type == "van") {
    return ObjectType::VEHICLE;
    // compatible with KITTI output - END
  } else {
    return ObjectType::UNKNOWN;
  }
}

}  // namespace perception
}  // namespace apollo