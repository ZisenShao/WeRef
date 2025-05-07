# WeRef
 
> The dataset is available at [here](https://huggingface.co/datasets/zisenshao/RoboCup-Referee-Gestures/tree/main). For any issue regarding the data pipeline or the dataset, please open an issue or directly contact the author via [email](mailto:zisenshao@outlook.com).

## Overview

WeRef is an open-source synthetic data generation pipeline for RoboCup Standard Platform League (SPL) referee gestures, built using the Webots simulator.

Below shows samples of how data was collected for dynamic gesture and single pose image:

<table>
  <tr>
    <td><img src="demos/sample_dynamic_gesture collection.gif" alt="Static gesture demo" /></td>
    <td><img src="demos/sample_image_data collection.gif" alt="Dynamic gesture demo" /></td>
  </tr>
</table>

### Dataset Overview
The generated dataset is organized and auto-labelled as below structure:
```bash
<dir>/
  <gesture>/
    <refereeModel>_<clothname>/
      <background>/
        presence_<robotOrNot>/
          <left_middle_right>/
            frame_0.jpg
            frame_1.jpg
            ...
```
| Gesture type            | Samples per **world variation** | Frames per **sample**                         | Frame breakdown                                                                                                                                               |
| ----------------------- | ------------------------------- | --------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Dynamic**             | 6                               |  **Full‑time:** 30<br> **Substitution:** 22 | Entire sequence captures the motion from start to finish                                                                                                      |
| **Static** (10 classes) | 6                               | 42                                            | *Frames 1 – 21*: transition from arms‑down to target pose  →  *Frames 22 – 42*: pose held steady, mimicking referees maintain the gesture in real competitions |


## Dependencies

* **Webots Simulator:** This codebase use Webots R2025a.

## Codebase Structure

* `controllers/`:
    * `nao_soccer_player/`: Main controller for data logging and simulation runtime configuration.
    * `bvh_animation/`: Controller for applying BVH motion for referee gestures.
* `libraries/bvh_util/`: Library for handling BVH files in Webots.
* `motions/`: Contains `.bvh` and `.motion` files for referee gestures and robot motion, respectively.
    * `generate_bvh.py`: Helper script to create/modify BVH files.
* `worlds/`: Webots world files (`.wbt`) defining different simulation scenes.
* `*.sh`: Shell scripts for automated data collection.

## Setup

1.  **Clone the Repository:**
    ```bash
    git clone git@github.com:ZisenShao/WeRef.git
    cd WeRef
    ```
2.  **Compile Controllers & Libraries:** Navigate into the `controllers/*` and `libraries/bvh_util` directories and compile the C code:
    ```bash
    cd libraries/bvh_util
    make
    cd ../../controllers/nao_soccer_player
    make
    cd ../bvh_animation
    make
    ```

## Usage

**Before running:**

* Edit the scripts to set the `WEBOTS_PATH` variable to the correct path of your Webots executable, the provided path is specified for Mac users.
* Uncomment the `// wb_camera_save_image(camera, file_path, 100);` in `nao_soccer_player.c` for saving images.

### 1. Running a Single Simulation

* Open one of the `.wbt` files located in the `worlds/` directory using the Webots application.
* Run the simulation. The `nao_soccer_player` controller will automatically randomize the environment according to its logic and save captured images.

### 2. Automated Data Collection

Three scripts are provided for batch data generation:

* `static_gestures_collection.sh`: Runs simulations for the 10 static referee gestures across different world files.
* `static_gestures_end_posture_collection.sh`: Runs simulations for the end posture of the 10 static referee gestures across different world files.
* `dynamic_gestures_collection.sh`: Runs simulations for the 2 dynamic referee gestures (Full Time, Substitution) across different world files.

**To run:**

```bash
chmod +x static_gestures_collection.sh
chmod +x static_gestures_end_posture_collection.sh
chmod +x dynamic_gestures_collection.sh
./static_gestures_collection.sh
# or
./static_gestures_end_posture_collection.sh
# or
./dynamic_gestures_collection.sh
