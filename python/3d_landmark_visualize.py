import numpy as np
import cv2
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os
import re

class UnrealDataLoader:
    def __init__(self):
        self.head_transform = None
        self.camera_transform = None
        self.fov = None
        self.resolution = None
        self.landmarks_3d = None
        self.landmarks_2d = None
    
    def parse_transform_string(self, transform_str):
        """Parse Unreal Engine transform string to matrix"""
        # Extract location
        loc_match = re.search(r'Location: X=([-\d.]+) Y=([-\d.]+) Z=([-\d.]+)', transform_str)
        location = np.array([float(loc_match.group(1)), float(loc_match.group(2)), float(loc_match.group(3))])
        
        # Extract rotation (Pitch, Yaw, Roll)
        rot_match = re.search(r'Rotation: P=([-\d.]+) Y=([-\d.]+) R=([-\d.]+)', transform_str)
        pitch = np.radians(float(rot_match.group(1)))
        yaw = np.radians(float(rot_match.group(2)))
        roll = np.radians(float(rot_match.group(3)))
        
        # Extract scale
        scale_match = re.search(r'Scale: X=([-\d.]+) Y=([-\d.]+) Z=([-\d.]+)', transform_str)
        scale = np.array([float(scale_match.group(1)), float(scale_match.group(2)), float(scale_match.group(3))])
        
        # Convert Unreal rotation to rotation matrix (Pitch, Yaw, Roll)
        # Unreal uses different axis convention
        rotation_matrix = self.euler_to_rotation_matrix(pitch, yaw, roll)
        
        # Create 4x4 transform matrix
        transform = np.eye(4)
        transform[:3, :3] = rotation_matrix * scale
        transform[:3, 3] = location
        
        return transform, location, rotation_matrix, scale
    
    def euler_to_rotation_matrix(self, pitch, yaw, roll):
        """Convert Euler angles to rotation matrix (Unreal convention)"""
        # Unreal Engine uses: Yaw (Z), Pitch (Y), Roll (X)
        cy = np.cos(yaw)
        sy = np.sin(yaw)
        cp = np.cos(pitch)
        sp = np.sin(pitch)
        cr = np.cos(roll)
        sr = np.sin(roll)
        
        # ZYX rotation order (Unreal convention)
        R = np.array([
            [cy*cp, cy*sp*sr - sy*cr, cy*sp*cr + sy*sr],
            [sy*cp, sy*sp*sr + cy*cr, sy*sp*cr - cy*sr],
            [-sp, cp*sr, cp*cr]
        ])
        
        return R
    
    def load_camera_data(self, camera_file_path):
        """Load camera pose and parameters"""
        with open(camera_file_path, 'r') as f:
            content = f.read()
        
        # Parse head transform
        head_section = re.search(r'Head:\n(.*?)\n\nCamera:', content, re.DOTALL)
        if head_section:
            head_transform_str = head_section.group(1)
            self.head_transform, _, _, _ = self.parse_transform_string("Location: " + head_transform_str)
        
        # Parse camera transform
        camera_section = re.search(r'Camera:\n(.*?)\n\nCamera Info:', content, re.DOTALL)
        if camera_section:
            camera_transform_str = camera_section.group(1)
            self.camera_transform, self.camera_location, self.camera_rotation, _ = self.parse_transform_string("Location: " + camera_transform_str)
        
        # Parse camera info
        fov_match = re.search(r'FOV: ([\d.]+)', content)
        self.fov = float(fov_match.group(1)) if fov_match else 90.0
        
        resolution_match = re.search(r'Resolution: (\d+)x(\d+)', content)
        if resolution_match:
            self.resolution = (int(resolution_match.group(1)), int(resolution_match.group(2)))
        else:
            self.resolution = (1920, 1080)
        
        print(f"Loaded camera data:")
        print(f"  FOV: {self.fov}")
        print(f"  Resolution: {self.resolution}")
        print(f"  Camera Location: {self.camera_location}")
    
    def load_landmarks(self, landmarks_file_path):
        """Load 3D landmarks and 2D projections"""
        landmarks_3d = []
        landmarks_2d = []
        
        with open(landmarks_file_path, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) >= 5:
                    # World coordinates
                    world_x, world_y, world_z = float(parts[0]), float(parts[1]), float(parts[2])
                    # Screen coordinates
                    screen_x, screen_y = float(parts[3]), float(parts[4])
                    
                    landmarks_3d.append([world_x, world_y, world_z])
                    landmarks_2d.append([screen_x, screen_y])
        
        self.landmarks_3d = np.array(landmarks_3d)
        self.landmarks_2d = np.array(landmarks_2d)
        
        print(f"Loaded {len(landmarks_3d)} landmarks")
    
    def project_3d_to_2d(self, points_3d):
        """Project 3D points to 2D screen coordinates using camera parameters"""
        if self.camera_transform is None or self.fov is None:
            raise ValueError("Camera data not loaded")
        
        # Convert FOV to focal length
        width, height = self.resolution
        focal_length = width / (2 * np.tan(np.radians(self.fov / 2)))
        
        # Camera intrinsic matrix
        K = np.array([
            [focal_length, 0, width / 2],
            [0, focal_length, height / 2],
            [0, 0, 1]
        ])
        
        # Transform points to camera coordinate system
        # Unreal to OpenCV coordinate conversion
        points_homogeneous = np.hstack([points_3d, np.ones((points_3d.shape[0], 1))])
        
        # Camera extrinsic matrix (world to camera)
        camera_inv = np.linalg.inv(self.camera_transform)
        points_camera = (camera_inv @ points_homogeneous.T).T[:, :3]
        
        # Convert Unreal coordinates to OpenCV coordinates
        # Unreal: X=forward, Y=right, Z=up
        # OpenCV: X=right, Y=down, Z=forward
        points_opencv = np.zeros_like(points_camera)
        points_opencv[:, 0] = points_camera[:, 1]   # Y -> X (right)
        points_opencv[:, 1] = -points_camera[:, 2]  # -Z -> Y (down)
        points_opencv[:, 2] = points_camera[:, 0]   # X -> Z (forward)
        
        # Project to 2D
        points_2d_homogeneous = (K @ points_opencv.T).T
        points_2d = points_2d_homogeneous[:, :2] / points_2d_homogeneous[:, 2:3]
        
        return points_2d
    
    def visualize_landmarks(self, show_original_projection=True, show_computed_projection=True):
        """Visualize landmarks in 2D and 3D"""
        if self.landmarks_3d is None:
            raise ValueError("Landmarks not loaded")
        
        # Create figure with subplots
        fig = plt.figure(figsize=(15, 10))
        
        # 2D visualization
        ax1 = fig.add_subplot(221)
        
        if show_original_projection and self.landmarks_2d is not None:
            ax1.scatter(self.landmarks_2d[:, 0], self.landmarks_2d[:, 1], 
                       c='red', s=1, alpha=0.6, label='Original UE4 Projection')
        
        if show_computed_projection:
            projected_2d = self.project_3d_to_2d(self.landmarks_3d)
            ax1.scatter(projected_2d[:, 0], projected_2d[:, 1], 
                       c='blue', s=1, alpha=0.6, label='Computed Projection')
        
        ax1.set_xlim(0, self.resolution[0])
        ax1.set_ylim(self.resolution[1], 0)  # Flip Y axis for screen coordinates
        ax1.set_xlabel('Screen X')
        ax1.set_ylabel('Screen Y')
        ax1.set_title('2D Landmark Projections')
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        
        # 3D visualization
        ax2 = fig.add_subplot(222, projection='3d')
        ax2.scatter(self.landmarks_3d[:, 0], self.landmarks_3d[:, 1], self.landmarks_3d[:, 2], 
                   c='green', s=1, alpha=0.6)
        
        # Plot camera position
        if self.camera_location is not None:
            ax2.scatter(self.camera_location[0], self.camera_location[1], self.camera_location[2], 
                       c='red', s=100, marker='^', label='Camera')
        
        ax2.set_xlabel('World X')
        ax2.set_ylabel('World Y')
        ax2.set_zlabel('World Z')
        ax2.set_title('3D Landmarks in World Space')
        ax2.legend()
        
        # Difference visualization (if both projections available)
        if show_original_projection and show_computed_projection and self.landmarks_2d is not None:
            ax3 = fig.add_subplot(223)
            projected_2d = self.project_3d_to_2d(self.landmarks_3d)
            
            # Calculate differences
            diff = projected_2d - self.landmarks_2d
            distances = np.linalg.norm(diff, axis=1)
            
            ax3.hist(distances, bins=50, alpha=0.7)
            ax3.set_xlabel('Projection Error (pixels)')
            ax3.set_ylabel('Count')
            ax3.set_title(f'Projection Error Distribution\nMean: {np.mean(distances):.2f} pixels')
            ax3.grid(True, alpha=0.3)
            
            # Error visualization
            ax4 = fig.add_subplot(224)
            scatter = ax4.scatter(self.landmarks_2d[:, 0], self.landmarks_2d[:, 1], 
                                 c=distances, s=1, alpha=0.6, cmap='viridis')
            ax4.set_xlim(0, self.resolution[0])
            ax4.set_ylim(self.resolution[1], 0)
            ax4.set_xlabel('Screen X')
            ax4.set_ylabel('Screen Y')
            ax4.set_title('Projection Error Heatmap')
            plt.colorbar(scatter, ax=ax4, label='Error (pixels)')
        
        plt.tight_layout()
        plt.show()
    
    def create_landmark_image(self, output_path=None):
        """Create an image with landmarks overlaid"""
        if self.landmarks_2d is None:
            raise ValueError("2D landmarks not available")
        
        # Create blank image
        img = np.zeros((self.resolution[1], self.resolution[0], 3), dtype=np.uint8)
        
        # Draw landmarks
        for point in self.landmarks_2d:
            x, y = int(point[0]), int(point[1])
            if 0 <= x < self.resolution[0] and 0 <= y < self.resolution[1]:
                cv2.circle(img, (x, y), 4, (0, 255, 0), -1)
        
        # Draw computed projection if available
        if self.landmarks_3d is not None:
            projected_2d = self.project_3d_to_2d(self.landmarks_3d)
            for point in projected_2d:
                x, y = int(point[0]), int(point[1])
                if 0 <= x < self.resolution[0] and 0 <= y < self.resolution[1]:
                    cv2.circle(img, (x, y), 1, (255, 0, 0), -1)
        
        if output_path:
            cv2.imwrite(output_path, img)
        
        return img

# Usage example
def main():
    # Initialize loader
    loader = UnrealDataLoader()
    
    # Load data (adjust paths as needed)
    camera_file = "../Saved/CapturedFrames/Mats_000300.txt"  # Your camera pose file
    landmarks_file = "../Saved/CapturedFrames/Landmarks_000300.txt"  # Your landmarks file
    
    try:
        # Load camera data
        loader.load_camera_data(camera_file)
        
        # Load landmarks
        loader.load_landmarks(landmarks_file)
        
        # Visualize
        loader.visualize_landmarks()
        
        # Create landmark image
        img = loader.create_landmark_image("landmarks_overlay.png")
        
        # Display with OpenCV
        cv2.imshow('Landmarks', img)
        cv2.waitKey(0)
        cv2.destroyAllWindows()
        
    except FileNotFoundError as e:
        print(f"File not found: {e}")
    except Exception as e:
        print(f"Error: {e}")

#if __name__ == "__main__":
    #main()

main()