#!/bin/bash

WEBOTS_PATH="/Applications/Webots.app/Contents/MacOS/webots"
WORLDS_DIR="worlds"

GESTURES=(
  "corner_kick_blue_end.bvh"
  "corner_kick_red_end.bvh"
  "goal_blue_end.bvh"
  "goal_red_end.bvh"
  "goal_kick_blue_end.bvh"
  "goal_kick_red_end.bvh"
  "kick_in_blue_end.bvh"
  "kick_in_red_end.bvh"
  "pushing_free_kick_blue_end.bvh"
  "pushing_free_kick_red_end.bvh"
)

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

for gesture in "${GESTURES[@]}"; do
  echo "=== Using gesture: $gesture ==="
  
  for wbt in "${WBT_FILES[@]}"; do
    echo "Modifying $wbt => $gesture"

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
