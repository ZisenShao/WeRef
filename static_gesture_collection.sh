#!/bin/bash

WEBOTS_PATH="/Applications/Webots.app/Contents/MacOS/webots"
WORLDS_DIR="worlds"

GESTURES=(
  "corner_kick_blue.bvh"
  "corner_kick_red.bvh"
  "goal_blue.bvh"
  "goal_red.bvh"
  "goal_kick_blue.bvh"
  "goal_kick_red.bvh"
  "kick_in_blue.bvh"
  "kick_in_red.bvh"
  "pushing_free_kick_blue.bvh"
  "pushing_free_kick_red.bvh"
)

# WBT files, excluding any "sandra_*.wbt".
WBT_FILES=(
  "anthony_dimlight_crowded.wbt"
  "robert_dimlight_crowded.wbt"
  "sophia_dimlight_crowded.wbt"

  "anthony_mediumlight_crowded_0.wbt"
  "robert_mediumlight_crowded_0.wbt"
  "sophia_mediumlight_crowded_0.wbt"

  "anthony_mediumlight_crowded_1.wbt"
  "robert_mediumlight_crowded_1.wbt"
  "sophia_mediumlight_crowded_1.wbt"

  "anthony_mediumlight_uncrowded_0.wbt"
  "robert_mediumlight_uncrowded_0.wbt"
  "sophia_mediumlight_uncrowded_0.wbt"

  "anthony_mediumlight_uncrowded_1.wbt"
  "robert_mediumlight_uncrowded_1.wbt"
  "sophia_mediumlight_uncrowded_1.wbt"

  "anthony_mediumlight_uncrowded_2.wbt"
  "robert_mediumlight_uncrowded_2.wbt"
  "sophia_mediumlight_uncrowded_2.wbt"

  "anthony_stronglight_crowded_0.wbt"
  "robert_stronglight_crowded_0.wbt"
  "sophia_stronglight_crowded_0.wbt"

  "anthony_stronglight_crowded_1.wbt"
  "robert_stronglight_crowded_1.wbt"
  "sophia_stronglight_crowded_1.wbt"
)

# Outer loop: for each gesture .bvh
for gesture in "${GESTURES[@]}"; do
  echo "=== Using gesture: $gesture ==="
  
  # Inner loop: process each .wbt file
  for wbt in "${WBT_FILES[@]}"; do
    echo "Modifying $wbt => $gesture"

    # In-place edit so that ../../motions/<anything>.bvh => ../../motions/$gesture
    sed -i '' "s|\(\.\./\.\./motions/\)[^\"]*\.bvh|\1$gesture|g" "$WORLDS_DIR/$wbt"

    echo "Running $wbt for 14 seconds..."
    "$WEBOTS_PATH" --batch --mode=fast "$WORLDS_DIR/$wbt" &
    WEBOTS_PID=$!

    sleep 12

    kill "$WEBOTS_PID" 2>/dev/null
    wait "$WEBOTS_PID" 2>/dev/null

    echo "Stopped $wbt"
  done

  echo "Done with gesture: $gesture"
done

echo "All gestures & worlds done."
