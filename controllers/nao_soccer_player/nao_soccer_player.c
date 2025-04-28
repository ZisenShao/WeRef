#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <webots/camera.h>
#include <webots/led.h>
#include <webots/motor.h>
#include <webots/robot.h>
#include <webots/supervisor.h>
#include <webots/utils/motion.h>

#ifdef _MSC_VER
#define snprintf sprintf_s
#endif

// --- Global Variables ---
static int time_step = -1;
static int frame_count = 0;
static const char *gesture = "";
static const char *clothName = "Cloth1";
static int obstacle_flag = -1; // -1: unknown, 0: absent, 1: present

// Webots Devices & Motion References
static WbDeviceTag CameraTop, CameraBottom;
static WbMotionRef currently_playing = NULL;

// Linked list structure for storing loaded motions
struct Motion {
  char *name;
  WbMotionRef ref;
  struct Motion *next;
} *motion_list = NULL;

// --- Function Implementations ---

/**
 * @brief Saves the current camera frame to a structured directory.
 */
static void store_frame_image(WbDeviceTag camera, int frame_index,
                              const char *gesture_name,
                              const char *referee_model,
                              const char *cloth_name,
                              const char *background,
                              int obstacleFlag,
                              const char *angle_position) {
  const char *presence_label;
  if (obstacleFlag == 1)
    presence_label = "presence_robot";
  else if (obstacleFlag == 0)
    presence_label = "presence_norobot";
  else
    presence_label = "presence_unknown";

  char dir_path[512];
  snprintf(dir_path, sizeof(dir_path),
           "images/%s/%s_%s/%s/%s/%s",
           gesture_name, referee_model, cloth_name,
           background, presence_label, angle_position);

  char mk_command[1024];
  snprintf(mk_command, sizeof(mk_command), "mkdir -p \"%s\"", dir_path);
  system(mk_command);

  char file_path[512];
  snprintf(file_path, sizeof(file_path), "%s/frame_%d.jpg", dir_path, frame_index);

  printf("Saving image to: %s\n", file_path);
  // wb_camera_save_image(camera, file_path, 100);
}

// ----------------------------------------------------------
// Camera / Motion Management
// ----------------------------------------------------------

/**
 * @brief Gets device tags for cameras and enables them.
 */
static void enable_cameras() {
  CameraTop = wb_robot_get_device("CameraTop");
  CameraBottom = wb_robot_get_device("CameraBottom");
  wb_camera_enable(CameraTop, time_step);
  wb_camera_enable(CameraBottom, time_step);
}

/**
 * @brief Loads all .motion files from the "../../motions" directory into a linked list.
 */
static void load_motion_list() {
  const char *motion_dir = "../../motions";
  DIR *d = opendir(motion_dir);
  if (d) {
    const struct dirent *dir;
    struct Motion *current_motion = NULL;
    while ((dir = readdir(d)) != NULL) {
      const char *name = dir->d_name;
      if (name[0] != '.') {
        struct Motion *new_motion = (struct Motion *)malloc(sizeof(struct Motion));
        if (current_motion == NULL)
          motion_list = new_motion;
        else
          current_motion->next = new_motion;
        current_motion = new_motion;
        current_motion->name = (char *)malloc(strlen(name) + 1);
        strcpy(current_motion->name, name);
        for (int i = 0; current_motion->name[i]; i++) {
          if (current_motion->name[i] == '.') {
            current_motion->name[i] = '\0';
            break;
          }
        }
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/%s.motion", motion_dir, current_motion->name);
        current_motion->ref = wbu_motion_new(filename);
        current_motion->next = NULL;
      }
    }
    closedir(d);
  } else {
       perror("Could not open motion directory");
  }
}

/**
 * @brief Frees the memory allocated for the motion list.
 */
static void free_motion_list() {
  while (motion_list) {
    free(motion_list->name);
    struct Motion *next = motion_list->next;
    free(motion_list);
    motion_list = next;
  }
}

/**
 * @brief Finds a loaded motion by its name.
 * @param name The name of the motion (without extension).
 * @return WbMotionRef if found, NULL otherwise.
 */
static WbMotionRef find_motion(const char *name) {
  struct Motion *motion = motion_list;
  while (motion) {
    if (strcmp(motion->name, name) == 0)
      return motion->ref;
    motion = motion->next;
  }
  fprintf(stderr, "Motion not found: %s\n", name);
  return NULL;
}

/**
 * @brief Starts playing a motion by name, stopping any currently playing motion.
 * @param name The name of the motion to play.
 */
static void start_motion(const char *name) {
  WbMotionRef motion = find_motion(name);
  if (currently_playing)
    wbu_motion_stop(currently_playing);

  wbu_motion_play(motion);
  currently_playing = motion;
}

// ----------------------------------------------------------
// Randomization Helpers
// ----------------------------------------------------------

/**
 * @brief Generates a random double within a specified range.
 */
static double rand_in_range(double min, double max) {
  return min + (max - min) * ((double)rand() / RAND_MAX);
}

/**
 * @brief Checks if point (x, y) is within half-circle C1.
 */
static int in_C1(double x, double y) {
  double dx = x - 3.0;
  double dist2 = dx * dx + y * y;
  return (dist2 <= 3.8 * 3.8) && (x <= 3.0);
}

/**
 * @brief Checks if point (x, y) is within half-circle C2.
 */
static int in_C2(double x, double y) {
  double dx = x - 3.0;
  double dist2 = dx * dx + y * y;
  return (dist2 <= 2.2 * 2.2) && (x <= 3.0);
}

/**
 * @brief Checks if point (x, y) is within circle C3.
 */
static int in_C3(double x, double y) {
  double dist2 = x * x + y * y;
  return (dist2 <= 0.8 * 0.8);
}

/**
 * @brief Calculates a random position for NAO RED 2.
 */
static void random_position_red2(double *px, double *py) {
  while (1) {
    double x = rand_in_range(-0.8, 3.0);
    double y = rand_in_range(0.0, 2.0);
    if (in_C1(x, y) && !in_C2(x, y) && !in_C3(x, y)) {
      *px = x;
      *py = y;
      return;
    }
  }
}

/**
 * @brief Calculates a random position for NAO BLUE 4.
 */
static void random_position_blue4(double *px, double *py) {
  while (1) {
    double x = rand_in_range(-0.8, 3.0);
    double y = rand_in_range(-2.0, 0.0);
    if (in_C1(x, y) && !in_C2(x, y) && !in_C3(x, y)) {
      *px = x;
      *py = y;
      return;
    }
  }
}

/**
 * @brief Calculates a random position for NAO RED 3.
 */
static void random_position_red3(double *px, double *py) {
  while (1) {
    double x = rand_in_range(-0.8, 0.8);
    double y = rand_in_range(-0.8, 0.8);
    if ((x * x + y * y) <= (0.8 * 0.8)) {
      *px = x;
      *py = y;
      return;
    }
  }
}

/**
 * @brief Sets the robot's rotation field to face the point (3, 0).
 */
static void set_facing_3_0(WbFieldRef rotation_field, double x, double y) {
  double dx = 3.0 - x;
  double dy = 0.0 - y;
  double angle = atan2(dy, dx);
  double axis_z = (y > 0.0) ? -1.0 : 1.0;
  angle = fabs(angle);
  double rot[4] = {0.0, 0.0, axis_z, angle};
  wb_supervisor_field_set_sf_rotation(rotation_field, rot);
}

/**
 * @brief Randomizes the position and visibility of the obstacle robot.
 */
static void randomize_obstacle_robot() {
  int obstacle_visible = (rand() < (RAND_MAX / 2)) ? 0 : 1;
  obstacle_flag = obstacle_visible;

  WbNodeRef obstacle_node = wb_supervisor_node_get_from_def("OBSTACLE_ROBOT");
  if (!obstacle_node) {
    fprintf(stderr, "Warning: Could not find node with DEF OBSTACLE_ROBOT.\n");
    return;
  }

  WbFieldRef translation_field = wb_supervisor_node_get_field(obstacle_node, "translation");
  WbFieldRef rotation_field    = wb_supervisor_node_get_field(obstacle_node, "rotation");
  if (!translation_field || !rotation_field) {
    fprintf(stderr, "Warning: Could not get translation/rotation for OBSTACLE_ROBOT.\n");
    return;
  }

  if (!obstacle_visible) {
    double no_obs_translation[3] = {-3.0, -4.0, 0.335};
    wb_supervisor_field_set_sf_vec3f(translation_field, no_obs_translation);

    double no_obs_rot[4] = {0.0, 0.0, 1.0, 0.0};
    wb_supervisor_field_set_sf_rotation(rotation_field, no_obs_rot);
  } else {
    double x, y;
    while (1) {
      x = rand_in_range(0.8, 3.0);
      y = rand_in_range(-2.2, 2.2);
      if (in_C2(x, y))
        break;
    }
    double obs_translation[3] = {x, y, 0.335};
    wb_supervisor_field_set_sf_vec3f(translation_field, obs_translation);

    double angle = rand_in_range(-3.14, 3.14);
    double obs_rot[4] = {0.0, 0.0, 1.0, angle};
    wb_supervisor_field_set_sf_rotation(rotation_field, obs_rot);
  }
}

/**
 * @brief Helper function to get the (x, y) translation of a node by its DEF name.
 */
static int get_robot_xy(const char *def_name, double *out_x, double *out_y) {
  WbNodeRef node = wb_supervisor_node_get_from_def(def_name);
  if (!node) return 0;
  WbFieldRef trans_field = wb_supervisor_node_get_field(node, "translation");
  if (!trans_field) return 0;
  const double *vals = wb_supervisor_field_get_sf_vec3f(trans_field);
  if (!vals) return 0;
  *out_x = vals[0];
  *out_y = vals[1];
  return 1;
}

/**
 * @brief Randomizes the ball's position, ensuring it's not too close to any robot.
 */
static void randomize_ball_position() {
  WbNodeRef ball_node = wb_supervisor_node_get_from_def("SOCCER_BALL");
  if (!ball_node) {
    fprintf(stderr, "Warning: Could not find node with DEF SOCCER_BALL.\n");
    return;
  }

  WbFieldRef translation_field = wb_supervisor_node_get_field(ball_node, "translation");
  if (!translation_field) {
    fprintf(stderr, "Warning: Could not get 'translation' field for SOCCER_BALL.\n");
    return;
  }

  double rx[4], ry[4];
  const char *robot_defs[4] = {"NAO RED 2", "NAO RED 3", "NAO BLUE 4", "OBSTACLE_ROBOT"};
  int valid_count = 0;

  for (int i = 0; i < 4; i++) {
    double x, y;
    if (get_robot_xy(robot_defs[i], &x, &y)) {
      rx[valid_count] = x;
      ry[valid_count] = y;
      valid_count++;
    }
  }

  double minDist = 0.3;
  while (1) {
    double bx = rand_in_range(-3.0, 3.0);
    double by = rand_in_range(-4.5, 4.5);

    int collide = 0;
    for (int i = 0; i < valid_count; i++) {
      double dx = bx - rx[i];
      double dy = by - ry[i];
      double dist = sqrt(dx * dx + dy * dy);
      if (dist < minDist) {
        collide = 1;
        break;
      }
    }

    if (!collide) {
      double new_trans[3] = {bx, by, 0.07};
      wb_supervisor_field_set_sf_vec3f(translation_field, new_trans);
      break;
    }
  }

  double zero_velocity[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  wb_supervisor_node_set_velocity(ball_node, zero_velocity);
}

/**
 * @brief Randomizes the direction and intensity (luminosity) of the background light.
 */
static void randomize_background_light() {
  WbNodeRef light_node = wb_supervisor_node_get_from_def("TEXTURED_BACKGROUND_LIGHT");
  if (!light_node) {
    fprintf(stderr, "Warning: Could not find node with DEF TEXTURED_BACKGROUND_LIGHT.\n");
    return;
  }

  WbFieldRef direction_field  = wb_supervisor_node_get_field(light_node, "direction");
  WbFieldRef luminosity_field = wb_supervisor_node_get_field(light_node, "luminosity");
  if (!direction_field || !luminosity_field) {
    fprintf(stderr, "Warning: Could not get direction or luminosity fields.\n");
    return;
  }

  double rx = rand_in_range(-2.0, 2.0);
  double ry = rand_in_range(-7.0, -0.7);
  double rz = rand_in_range(-2.0, 2.0);
  double new_luminosity = rand_in_range(0.0, 3.0);

  double new_dir[3] = {rx, ry, rz};
  wb_supervisor_field_set_sf_vec3f(direction_field, new_dir);
  wb_supervisor_field_set_sf_float(luminosity_field, new_luminosity);
}

// ----------------------------------------------------------
// Controller "main"
// ----------------------------------------------------------
int main() {
  wb_robot_init();
  time_step = wb_robot_get_basic_time_step();

  srand((unsigned)(time(NULL) ^ (unsigned)clock()));

  const char *name = wb_robot_get_name();

  const char *full_world_path = wb_robot_get_world_path();
  if (!full_world_path) {
    printf("Failed to get the world path.\n");
    wb_robot_cleanup();
    return 1;
  }

  const char *last_slash = strrchr(full_world_path, '/');
  const char *filename    = last_slash ? last_slash + 1 : full_world_path;

  char world_name[256];
  strncpy(world_name, filename, sizeof(world_name));
  world_name[sizeof(world_name) - 1] = '\0';

  char *dot = strrchr(world_name, '.');
  if (dot && strcmp(dot, ".wbt") == 0)
    *dot = '\0';

  printf("World name (derived): %s\n", world_name);

  char wbt_path[512];
  snprintf(wbt_path, sizeof(wbt_path), "../../worlds/%s.wbt", world_name);

  FILE *fp = fopen(wbt_path, "r");
  if (!fp) {
    printf("Failed to open world file: %s\n", wbt_path);
    wb_robot_cleanup();
    return 1;
  }

  printf("Opened world file: %s\n", wbt_path);

  char gesture_found[128] = "";

  char line[1024];
  int  found_flag_line = 0;
  while (fgets(line, sizeof(line), fp)) {
    line[strcspn(line, "\r\n")] = '\0';

    if (!found_flag_line) {
      if (strstr(line, "-f")) {
        found_flag_line = 1;
      }
    } else {
      char *start = strstr(line, "motions/");
      if (start) {
        start += strlen("motions/");
        char *end = strstr(start, ".bvh");
        if (end) {
          *end = '\0';
          strncpy(gesture_found, start, sizeof(gesture_found) - 1);
          gesture_found[sizeof(gesture_found) - 1] = '\0';
          printf("Extracted gesture: %s\n", gesture_found);
          break;
        }
      }
      break;
    }
  }

  fclose(fp);

  if (strlen(gesture_found) == 0) {
    printf("WARNING: Could not find a .bvh gesture name in %s\n", wbt_path);
  } else {
    gesture = gesture_found;
    printf("Using gesture: %s\n", gesture);
  }

  char refereeModel[128] = "unknownReferee";
  char background[128]   = "unknownBackground";
  {
    char temp[256];
    strncpy(temp, world_name, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';

    char *underscore = strchr(temp, '_');
    if (underscore) {
      *underscore = '\0';
      strncpy(refereeModel, temp, sizeof(refereeModel));
      refereeModel[sizeof(refereeModel) - 1] = '\0';

      const char *bg_part = underscore + 1;
      strncpy(background, bg_part, sizeof(background));
      background[sizeof(background) - 1] = '\0';
    }
  }

  load_motion_list();

  bool isRed2  = strcmp(name, "NAO RED 2")  == 0;
  bool isRed3  = strcmp(name, "NAO RED 3")  == 0;
  bool isBlue4 = strcmp(name, "NAO BLUE 4") == 0;

  if (isRed2 || isRed3 || isBlue4) {
    enable_cameras();
    printf("Camera enabled for %s\n", name);
  }

  char position[16] = "";
  if (isRed2)
    strcpy(position, "left");
  else if (isRed3)
    strcpy(position, "middle");
  else if (isBlue4)
    strcpy(position, "right");

  start_motion("static_image_collection");

  double startTime = wb_robot_get_time();

  if (strcmp(gesture, "full_time") != 0 && strcmp(gesture, "substitution") != 0) {
    while (wb_robot_get_time() < startTime + 3.0) {
      if (wb_robot_step(time_step) == -1)
        break;
    }

    while (true) {
      if (strcmp(name, "NAO RED 2") == 0) {
        randomize_background_light();
      }

      if (strcmp(name, "OBSTACLE ROBOT") == 0) {
        randomize_obstacle_robot();
      }

      if (isRed2 || isRed3 || isBlue4) {
        WbNodeRef my_node = wb_supervisor_node_get_self();
        if (my_node) {
          WbFieldRef translation_field = wb_supervisor_node_get_field(my_node, "translation");
          WbFieldRef rotation_field    = wb_supervisor_node_get_field(my_node, "rotation");
          if (translation_field && rotation_field) {
            double rx, ry;
          if (isRed3) {
              random_position_red3(&rx, &ry);
          } else if (isRed2) {
              random_position_red2(&rx, &ry);
          } else {
              random_position_blue4(&rx, &ry);
          }
            double new_translation[3] = {rx, ry, 0.335};
            wb_supervisor_field_set_sf_vec3f(translation_field, new_translation);
            set_facing_3_0(rotation_field, rx, ry);
          }
        }
      }

      randomize_ball_position();

      if (isRed2 || isRed3 || isBlue4) {
        WbNodeRef obs_node = wb_supervisor_node_get_from_def("OBSTACLE_ROBOT");
        if (obs_node) {
          WbFieldRef obs_trans_field = wb_supervisor_node_get_field(obs_node, "translation");
          if (obs_trans_field) {
            const double *vals = wb_supervisor_field_get_sf_vec3f(obs_trans_field);
            if (vals) {
              double dx = vals[0] + 3.0;
              double dy = vals[1] + 4.0;
              double dist = sqrt(dx * dx + dy * dy);
              if (dist < 0.1)
                obstacle_flag = 0;
              else
                obstacle_flag = 1;
            }
          }
        }
      }

      if (isRed2 || isRed3 || isBlue4) {
        store_frame_image(CameraTop,
                          frame_count,
                          gesture,
                          refereeModel, clothName,
                          background,
                          obstacle_flag,
                          position);
        frame_count++;
      }

      if (wb_robot_step(time_step) == -1)
        break;
    }
  }
  else if (strcmp(gesture, "full_time") == 0) {
    double nextRandomTime    = startTime + 1.26;
    const double randomInterval = 0.6;
    bool didInitialRandom = false;

    while (true) {
      double currentTime = wb_robot_get_time();
      if (isRed2 || isRed3 || isBlue4) {
        WbNodeRef obs_node = wb_supervisor_node_get_from_def("OBSTACLE_ROBOT");
        if (obs_node) {
          WbFieldRef obs_trans_field = wb_supervisor_node_get_field(obs_node, "translation");
          if (obs_trans_field) {
            const double *vals = wb_supervisor_field_get_sf_vec3f(obs_trans_field);
            if (vals) {
              double dx = vals[0] + 3.0;
              double dy = vals[1] + 4.0;
              double dist = sqrt(dx * dx + dy * dy);
              if (dist < 0.1)
                obstacle_flag = 0;
              else
                obstacle_flag = 1;
            }
          }
        }
      }

      if (currentTime >= nextRandomTime) {
        if (strcmp(name, "NAO RED 2") == 0) {
          randomize_background_light();
        }
        if (strcmp(name, "OBSTACLE ROBOT") == 0) {
          randomize_obstacle_robot();
        }
        if (isRed2 || isRed3 || isBlue4) {
          WbNodeRef my_node = wb_supervisor_node_get_self();
          if (my_node) {
            WbFieldRef translation_field = wb_supervisor_node_get_field(my_node, "translation");
            WbFieldRef rotation_field    = wb_supervisor_node_get_field(my_node, "rotation");
            if (translation_field && rotation_field) {
              double rx, ry;
              if (isRed3)
                random_position_red3(&rx, &ry);
              else if (isRed2)
                random_position_red2(&rx, &ry);
              else
                random_position_blue4(&rx, &ry);
              double new_translation[3] = {rx, ry, 0.335};
              wb_supervisor_field_set_sf_vec3f(translation_field, new_translation);
              set_facing_3_0(rotation_field, rx, ry);
            }
          }
        }
        randomize_ball_position();

        didInitialRandom = true;

        nextRandomTime += randomInterval;
      }

      if ((isRed2 || isRed3 || isBlue4) && currentTime >= (startTime + 1.26) && didInitialRandom) {
        store_frame_image(CameraTop,
                          frame_count,
                          gesture,
                          refereeModel, clothName,
                          background,
                          obstacle_flag,
                          position);
        frame_count++;
      }

      if (wb_robot_step(time_step) == -1)
        break;
    }
  }

  else if (strcmp(gesture, "substitution") == 0) {
    double nextRandomTime    = startTime + 0.94;
    const double randomInterval = 0.44;
    bool didInitialRandom = false;

    while (true) {
      double currentTime = wb_robot_get_time();
      if (isRed2 || isRed3 || isBlue4) {
        WbNodeRef obs_node = wb_supervisor_node_get_from_def("OBSTACLE_ROBOT");
        if (obs_node) {
          WbFieldRef obs_trans_field = wb_supervisor_node_get_field(obs_node, "translation");
          if (obs_trans_field) {
            const double *vals = wb_supervisor_field_get_sf_vec3f(obs_trans_field);
            if (vals) {
              double dx = vals[0] + 3.0;
              double dy = vals[1] + 4.0;
              double dist = sqrt(dx * dx + dy * dy);
              if (dist < 0.1)
                obstacle_flag = 0;
              else
                obstacle_flag = 1;
            }
          }
        }
      }

      if (currentTime >= nextRandomTime) {
        if (strcmp(name, "NAO RED 2") == 0) {
          randomize_background_light();
        }
        if (strcmp(name, "OBSTACLE ROBOT") == 0) {
          randomize_obstacle_robot();
        }
        if (isRed2 || isRed3 || isBlue4) {
          WbNodeRef my_node = wb_supervisor_node_get_self();
          if (my_node) {
            WbFieldRef translation_field = wb_supervisor_node_get_field(my_node, "translation");
            WbFieldRef rotation_field    = wb_supervisor_node_get_field(my_node, "rotation");
            if (translation_field && rotation_field) {
              double rx, ry;
              if (isRed3)
                random_position_red3(&rx, &ry);
              else if (isRed2)
                random_position_red2(&rx, &ry);
              else
                random_position_blue4(&rx, &ry);
              double new_translation[3] = {rx, ry, 0.335};
              wb_supervisor_field_set_sf_vec3f(translation_field, new_translation);
              set_facing_3_0(rotation_field, rx, ry);
            }
          }
        }
        randomize_ball_position();

        didInitialRandom = true;

        nextRandomTime += randomInterval;
      }

      if ((isRed2 || isRed3 || isBlue4) && currentTime >= (startTime + 0.94) && didInitialRandom) {
        store_frame_image(CameraTop,
                          frame_count,
                          gesture,
                          refereeModel, clothName,
                          background,
                          obstacle_flag,
                          position);
        frame_count++;
      }

      if (wb_robot_step(time_step) == -1)
        break;
    }
  }

  wb_robot_cleanup();
  free_motion_list();
  return 0;
}