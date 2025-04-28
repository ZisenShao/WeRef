import os
import sys


# ---------------------- Utility functions ---------------------- #

def format_channel_val(val: float) -> str:
    """
    Format a float so that:
      - 0.0 prints as "0"
      - Integers (e.g. 90.0) print as "90"
      - Other floats print with minimal decimals while preserving up to 6 decimals.
    """
    EPS = 1e-12
    if abs(val) < EPS:
        return "0"
    rounded = round(val)
    if abs(val - rounded) < EPS:
        return str(int(rounded))
    s = f"{val:.6f}".rstrip('0').rstrip('.')
    return s

def parse_motion_line(line: str) -> list[float]:
    """Parses a line of 96 channels into a list of floats."""
    parts = line.strip().split()
    floats = list(map(float, parts))
    if len(floats) != 96:
        raise ValueError(f"Expected 96 channels, got {len(floats)}")
    return floats

def read_bvh(filename: str):
    """
    Reads an existing BVH file and returns:
      - skeleton_lines: list of lines for everything up to 'MOTION'
      - frames: list of frames (each a list of 96 floats)
      - frame_time: the value parsed from 'Frame Time: X'
    """
    skeleton_lines = []
    frames = []
    frame_time = None

    with open(filename, 'r') as f:
        lines = f.readlines()
    
    # Find the line that starts with 'MOTION'
    motion_index = None
    for i, line in enumerate(lines):
        if line.strip().upper() == "MOTION":
            motion_index = i
            break
    
    if motion_index is None:
        raise ValueError("BVH file has no MOTION block.")
    
    # Everything up to MOTION is skeleton:
    skeleton_lines = lines[:motion_index]
    
    # After MOTION, next lines: "Frames: X" then "Frame Time: X"
    motion_section = lines[motion_index+1:]
    # Clean out empty lines:
    motion_section = [ln.strip() for ln in motion_section if ln.strip()]

    if len(motion_section) < 3:
        raise ValueError("BVH file does not appear to have correct motion lines (Frames, Frame Time, frames...).")

    # Parse "Frames: X"
    if not motion_section[0].upper().startswith("FRAMES:"):
        raise ValueError("Expected 'Frames: <num>' line after 'MOTION'.")
    num_frames = int(motion_section[0].split(":")[1].strip())

    # Parse "Frame Time: X"
    if not motion_section[1].lower().startswith("frame time:"):
        raise ValueError("Expected 'Frame Time: <value>' line.")
    frame_time = float(motion_section[1].split(":")[1].strip())

    # Next lines are all frame data
    frame_lines = motion_section[2:]
    if len(frame_lines) != num_frames:
        raise ValueError(
            f"Expected {num_frames} frame lines, but found {len(frame_lines)} in file."
        )
    
    # Parse each line into a list of floats
    for ln in frame_lines:
        frames.append(parse_motion_line(ln))
    
    return skeleton_lines, frames, frame_time

def write_bvh(filename: str, skeleton_lines: list[str], frames: list[list[float]], frame_time: float):
    """
    Writes a BVH file with the given skeleton lines, frames, and frame_time.
    Assumes each frame is a list of 96 floats.
    """
    num_frames = len(frames)
    
    # Write the file
    with open(filename, 'w') as f:
        # Write skeleton exactly as read
        for line in skeleton_lines:
            f.write(line.rstrip('\n') + '\n')
        
        # If the skeleton did not already have a trailing "MOTION", add it:
        last_line_stripped = skeleton_lines[-1].strip().upper()
        if last_line_stripped != "MOTION":
            f.write("MOTION\n")

        f.write(f"Frames: {num_frames}\n")
        f.write(f"Frame Time: {frame_time}\n")
        
        for frame_data in frames:
            if len(frame_data) != 96:
                raise ValueError("Each frame must have exactly 96 channels.")
            # Format each float accordingly
            str_channels = [format_channel_val(v) for v in frame_data]
            line = " ".join(str_channels)
            f.write(line + "\n")

def get_three_floats(prompt: str) -> list[float]:
    """
    Prompt the user for 3 floats (e.g. "Enter 3 floats for LeftShoulder"),
    repeating until valid input is provided. Return them as a list of floats.
    """
    while True:
        inp = input(f"Enter 3 floats for {prompt}, separated by spaces: ")
        try:
            vals = list(map(float, inp.strip().split()))
            if len(vals) == 3:
                return vals
        except ValueError:
            pass
        print("Invalid input. Please enter exactly 3 numeric values.")


# Channel groups for the 3-channel rotations we let users modify.
# 1-based channel indices in the spec -> 0-based for Python.
channel_groups = [
    ("LeftShoulder",      (54, 55, 56)),
    ("LeftArm",           (57, 58, 59)),
    ("LeftForeArm",       (60, 61, 62)),
    ("LeftHand",          (63, 64, 65)),
    ("LeftFingerBase",    (66, 67, 68)),
    ("LeftHandIndex1",    (69, 70, 71)),
    ("LThumb",            (72, 73, 74)),
    ("RightShoulder",     (75, 76, 77)),
    ("RightArm",          (78, 79, 80)),
    ("RightForeArm",      (81, 82, 83)),
    ("RightHand",         (84, 85, 86)),
    ("RightFingerBase",   (87, 88, 89)),
    ("RightHandIndex1",   (90, 91, 92)),
    ("RThumb",            (93, 94, 95)),
]

# ---------------------- Main generation function ---------------------- #

def generate_bvh():
    # The exact skeleton in a single string:
    bvh_hierarchy = """HIERARCHY
ROOT Hips
{
    OFFSET 0.00000 0.00000 0.00000
    CHANNELS 6 Xposition Yposition Zposition Zrotation Yrotation Xrotation 
    JOINT LHipJoint
    {
        OFFSET 0 0 0
        CHANNELS 3 Zrotation Yrotation Xrotation
        JOINT LeftUpLeg
        {
            OFFSET 1.85590 -1.73949 0.84976
            CHANNELS 3 Zrotation Yrotation Xrotation
            JOINT LeftLeg
            {
                OFFSET 2.36836 -6.50702 0.00000
                CHANNELS 3 Zrotation Yrotation Xrotation
                JOINT LeftFoot
                {
                    OFFSET 2.53268 -6.95849 0.00000
                    CHANNELS 3 Zrotation Yrotation Xrotation
                    JOINT LeftToeBase
                    {
                        OFFSET 0.15935 -0.43781 1.94506
                        CHANNELS 3 Zrotation Yrotation Xrotation
                        End Site
                        {
                            OFFSET 0.00000 0.00000 1.00661
                        }
                    }
                }
            }
        }
    }
    JOINT RHipJoint
    {
        OFFSET 0 0 0
        CHANNELS 3 Zrotation Yrotation Xrotation
        JOINT RightUpLeg
        {
            OFFSET -1.68297 -1.73949 0.84976
            CHANNELS 3 Zrotation Yrotation Xrotation
            JOINT RightLeg
            {
                OFFSET -2.44709 -6.72334 0.00000
                CHANNELS 3 Zrotation Yrotation Xrotation
                JOINT RightFoot
                {
                    OFFSET -2.43843 -6.69953 0.00000
                    CHANNELS 3 Zrotation Yrotation Xrotation
                    JOINT RightToeBase
                    {
                        OFFSET -0.20854 -0.57295 2.02172
                        CHANNELS 3 Zrotation Yrotation Xrotation
                        End Site
                        {
                            OFFSET -0.00000 0.00000 1.05594
                        }
                    }
                }
            }
        }
    }
    JOINT LowerBack
    {
        OFFSET 0 0 0
        CHANNELS 3 Zrotation Yrotation Xrotation
        JOINT Spine
        {
            OFFSET -0.01560 2.23971 -0.03712
            CHANNELS 3 Zrotation Yrotation Xrotation
            JOINT Spine1
            {
                OFFSET 0.05490 2.19225 -0.19086
                CHANNELS 3 Zrotation Yrotation Xrotation
                JOINT Neck
                {
                    OFFSET 0 0 0
                    CHANNELS 3 Zrotation Yrotation Xrotation
                    JOINT Neck1
                    {
                        OFFSET -0.12403 1.43168 0.27585
                        CHANNELS 3 Zrotation Yrotation Xrotation
                        JOINT Head
                        {
                            OFFSET 0.17855 1.46173 -0.32578
                            CHANNELS 3 Zrotation Yrotation Xrotation
                            End Site
                            {
                                OFFSET 0.07217 1.51590 -0.14537
                            }
                        }
                    }
                }
                JOINT LeftShoulder
                {
                    OFFSET 0 0 0
                    CHANNELS 3 Zrotation Yrotation Xrotation
                    JOINT LeftArm
                    {
                        OFFSET 3.27650 0.85634 0.05396
                        CHANNELS 3 Zrotation Yrotation Xrotation
                        JOINT LeftForeArm
                        {
                            OFFSET 4.96755 0.00000 0.00000
                            CHANNELS 3 Zrotation Yrotation Xrotation
                            JOINT LeftHand
                            {
                                OFFSET 3.35751 0.00000 0.00000
                                CHANNELS 3 Zrotation Yrotation Xrotation
                                JOINT LeftFingerBase
                                {
                                    OFFSET 0 0 0
                                    CHANNELS 3 Zrotation Yrotation Xrotation
                                    JOINT LeftHandIndex1
                                    {
                                        OFFSET 0.79697 0.00000 0.00000
                                        CHANNELS 3 Zrotation Yrotation Xrotation
                                        End Site
                                        {
                                            OFFSET 0.64254 0.00000 0.00000
                                        }
                                    }
                                }
                                JOINT LThumb
                                {
                                    OFFSET 0 0 0
                                    CHANNELS 3 Zrotation Yrotation Xrotation
                                    End Site
                                    {
                                        OFFSET 0.65235 0.00000 0.65235
                                    }
                                }
                            }
                        }
                    }
                }
                JOINT RightShoulder
                {
                    OFFSET 0 0 0
                    CHANNELS 3 Zrotation Yrotation Xrotation
                    JOINT RightArm
                    {
                        OFFSET -3.24434 1.02034 0.34688
                        CHANNELS 3 Zrotation Yrotation Xrotation
                        JOINT RightForeArm
                        {
                            OFFSET -5.21859 0.00000 0.00000
                            CHANNELS 3 Zrotation Yrotation Xrotation
                            JOINT RightHand
                            {
                                OFFSET -3.36504 0.00000 0.00000
                                CHANNELS 3 Zrotation Yrotation Xrotation
                                JOINT RightFingerBase
                                {
                                    OFFSET 0 0 0
                                    CHANNELS 3 Zrotation Yrotation Xrotation
                                    JOINT RightHandIndex1
                                    {
                                        OFFSET -0.62044 0.00000 0.00000
                                        CHANNELS 3 Zrotation Yrotation Xrotation
                                        End Site
                                        {
                                            OFFSET -0.50022 0.00000 0.00000
                                        }
                                    }
                                }
                                JOINT RThumb
                                {
                                    OFFSET 0 0 0
                                    CHANNELS 3 Zrotation Yrotation Xrotation
                                    End Site
                                    {
                                        OFFSET -0.50786 0.00000 0.50786
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
    """

    # Ask user for output filename
    filename = input("Enter the output BVH filename (e.g. 'output.bvh'): ").strip()
    if not filename.endswith(".bvh"):
        filename += ".bvh"
    
    # Check if file already exists
    if os.path.exists(filename):
        print(f"File '{filename}' already exists.")
        choice = input("Overwrite (O) or Modify (M) this file? [O/M]: ").strip().lower()
        
        if choice == 'm':
            # Modify existing BVH
            try:
                skeleton_lines, frames, frame_time = read_bvh(filename)
            except Exception as e:
                print(f"Could not parse existing BVH file. Error:\n{e}")
                print("Aborting.")
                return
            
            # Let user modify frames
            num_frames = len(frames)
            print(f"\nThere are {num_frames} frames in '{filename}'.")
            
            while True:
                # Ask which frame to modify
                frm_str = input(f"\nEnter frame number to modify (1..{num_frames}, or 0 to finish): ")
                try:
                    frm_idx = int(frm_str)
                except ValueError:
                    print("Please enter a valid integer.")
                    continue

                if frm_idx == 0:
                    # user done
                    break
                if not (1 <= frm_idx <= num_frames):
                    print(f"Frame must be in [1..{num_frames}].")
                    continue
                
                # Convert to 0-based
                frm_idx_0 = frm_idx - 1

                # Show menu of channel groups
                print("\nWhich 3-channel group do you want to modify?\n")
                for i, (name, _) in enumerate(channel_groups, start=1):
                    print(f" {i}) {name}")
                print(" 0) Cancel this modification")
                
                grp_str = input("\nEnter the group number: ")
                try:
                    grp_idx = int(grp_str)
                except ValueError:
                    print("Please enter a valid integer.")
                    continue
                
                if grp_idx == 0:
                    continue
                if not (1 <= grp_idx <= len(channel_groups)):
                    print("Invalid group choice.")
                    continue
                
                group_name, channels_tuple = channel_groups[grp_idx-1]
                
                # Show the original 3 values for this group, for reference
                old_vals = [frames[frm_idx_0][c] for c in channels_tuple]
                old_vals_str = " ".join(format_channel_val(v) for v in old_vals)
                print(f"Current {group_name} values: {old_vals_str}")
                
                # Prompt for new 3 floats
                new_vals = get_three_floats(group_name)
                
                # Apply to the frame
                for c, nv in zip(channels_tuple, new_vals):
                    frames[frm_idx_0][c] = nv
                
                print(f"Frame {frm_idx}, group '{group_name}' updated from '{old_vals_str}' to '{new_vals}'.")
            
            # After user finishes modifications, write BVH
            write_bvh(filename, skeleton_lines, frames, frame_time)
            print(f"\nBVH file '{filename}' updated successfully.")
            return
        
        else:
            print("Overwriting the file...")
            # Fall through to the "generate new BVH" flow
    # end if file exists

    # If file does not exist or user chooses to overwrite:
    try:
        num_frames = int(input("Enter number of frames: "))
    except ValueError:
        print("Invalid number of frames. Defaulting to 2.")
        num_frames = 2

    # We'll store each frame as a list of 96 floats.
    # - The first 3 are root position => 7.4882, 15.9816, -35.4705
    # - Next 51 are zero => channels 4..54
    # - Then 14 groups of 3 channels each => 42 channels => total 96.

    all_frames = []
    for frame_idx in range(num_frames):
        print(f"\n--- Creating frame {frame_idx+1}/{num_frames} ---")
        
        frame_channels = []
        
        # 1-3: fixed root position
        frame_channels.extend([7.4882, 15.9816, -35.4705])
        
        # 4-54: zero
        frame_channels.extend([0.0]*51)
        
        # Left side
        left_shoulder   = get_three_floats("LeftShoulder (channels 55-57)")
        left_arm        = get_three_floats("LeftArm (channels 58-60)")
        left_forearm    = get_three_floats("LeftForeArm (channels 61-63)")
        left_hand       = get_three_floats("LeftHand (channels 64-66)")
        left_fingerbase = get_three_floats("LeftFingerBase (channels 67-69)")
        left_handindex1 = get_three_floats("LeftHandIndex1 (channels 70-72)")
        lthumb          = get_three_floats("LThumb (channels 73-75)")
        
        # Right side
        right_shoulder   = get_three_floats("RightShoulder (channels 76-78)")
        right_arm        = get_three_floats("RightArm (channels 79-81)")
        right_forearm    = get_three_floats("RightForeArm (channels 82-84)")
        right_hand       = get_three_floats("RightHand (channels 85-87)")
        right_fingerbase = get_three_floats("RightFingerBase (channels 88-90)")
        right_handindex1 = get_three_floats("RightHandIndex1 (channels 91-93)")
        rthumb           = get_three_floats("RThumb (channels 94-96)")
        
        # Append them in order
        frame_channels.extend(left_shoulder)
        frame_channels.extend(left_arm)
        frame_channels.extend(left_forearm)
        frame_channels.extend(left_hand)
        frame_channels.extend(left_fingerbase)
        frame_channels.extend(left_handindex1)
        frame_channels.extend(lthumb)
        
        frame_channels.extend(right_shoulder)
        frame_channels.extend(right_arm)
        frame_channels.extend(right_forearm)
        frame_channels.extend(right_hand)
        frame_channels.extend(right_fingerbase)
        frame_channels.extend(right_handindex1)
        frame_channels.extend(rthumb)
        
        if len(frame_channels) != 96:
            print("Error: This frame does not have 96 channels. Exiting.")
            sys.exit(1)
        
        all_frames.append(frame_channels)
    
    # Default frame time
    frame_time = 0.0083333  # 1/120 second, for example

    # In the overwrite case, we didn’t read an existing skeleton,
    # so we build skeleton_lines from the multiline string above.
    skeleton_lines = [ln + "\n" for ln in bvh_hierarchy.strip().split("\n")]

    write_bvh(filename, skeleton_lines, all_frames, frame_time)
    print(f"\nBVH file '{filename}' successfully written with {num_frames} frame(s).")


if __name__ == "__main__":
    generate_bvh()
