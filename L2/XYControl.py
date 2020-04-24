import logging
import time
from abc import ABC, abstractmethod
from L2.Utility import UtilityControl, UtilityFactory


class XYAbstraction(ABC):
    """
    Utility class for moving an automated microscope stage. Values should be recorded in millimeters.

    Specialized functions include:
    set_xy = sets the absolute position of the stage
    read_xy = reads the absolute position of the stage
    set_x = sets the absolute position of x axis
    set_y = sets the absolute position o f they axis
    set_rel_xy = sets the relative position of the xy_stage, similar to jog xy
    set_rel_x = sets relative position of the x axis
    set_rel_y = sets the relative position of the y axis
    set_home = sets the home position for L2 and above
    go_home = moves to the home position
    stop = stopes xy stage movement
    wait_for_move = waits for the stage to stop moving
    """

    def __init__(self, controller, role):
        self.controller = controller
        self.role = role
        self._scale = 1
        self._x_inversion = 1
        self._y_inversion = 1
        self.pos = [0, 0]
        self._acceleration = 10 # mm/s2
        self._velocity = 5 # mm/s
        self.velocity_max = 5
        self.acceleration = 20
        self.jerk = 5


    def _scale_values(self, xy):
        xy = [x / self._scale for x in xy]
        xy[0] *= self._x_inversion
        xy[1] *= self._y_inversion
        return xy

    def _invert_scale(self, xy):
        xy[0] *= self._x_inversion
        xy[1] *= self._y_inversion
        xy = [x * self._scale for x in xy]
        return xy

    @abstractmethod
    def get_velocity(self):
        """

        :return:
        """
        return self._velocity

    @abstractmethod
    def get_accleration(self):
        """
        Micromanager does not have a way to retrieve the Stage acceleration, so we can make an estimate.

        :return:
        """
        return self._acceleration

    @abstractmethod
    def set_acceleration(self, acceleration):
        """
        Set the accerlation of the XY stage
        :return:
        """
        pass

    @abstractmethod
    def set_velocity(self, velocity):
        """
        Set the velocity of the XY stage
        :return:
        """
        pass

    @abstractmethod
    def read_xy(self):
        pass

    @abstractmethod
    def set_xy(self, xy):
        pass

    def set_x(self, x):
        xy = self.read_xy()
        xy[0] = x
        return self.set_xy(xy)

    def set_y(self, y):
        xy = self.read_xy()
        xy[1] = y
        return self.set_xy(xy)

    @abstractmethod
    def set_rel_xy(self, rel_xy):
        pass

    def set_rel_x(self, rel_x):
        return self.set_rel_xy([rel_x, 0])

    def set_rel_y(self, rel_y):
        return self.set_rel_xy([0, rel_y])

    def set_home(self):
        self.home = self.read_xy()

    def go_home(self):
        return self.set_xy(self.home)

    def stop(self):
        return self.set_rel_xy(0, 0)

    def get_status(self):
        """Get the position of the XY stage, satisfies the utility control get_status command"""
        return {'xy': self.pos}

    def wait_for_move(self, tolerance=10):
        """ Waits for the stage to stop moving
        tolerance is the acceptable distance from the target is it okay to consider the stage stopped
        returns the current position
        """
        time.sleep(0.1)
        prev_pos = self.read_xy()
        current_pos = [prev_pos[0] + 1, prev_pos[1]]
        # Update position while moving
        while abs(prev_pos[0] - current_pos[0]) > tolerance or abs(prev_pos[1] - current_pos[1]) > tolerance:
            time.sleep(0.05)
            prev_pos = current_pos
            current_pos = self.read_xy()
        return current_pos


class MicroManagerXY(XYAbstraction, UtilityControl):

    def __init__(self, controller, role):
        super().__init__(controller, role)
        self._dev_name = 'N/A'
        self._scale = 0.001 # Micromanager records units in um

    def _get_xy_device(self):
        """
        Find the XY stage device name, can change depending on the adapter/library being used so we assume
        that all xy stages will have xy somehwere in the name and return the first instance of that device
        :return:
        """
        if self._dev_name == "N/A":
            self._dev_name = self.controller.send_command('core,get_xy_name').decode()
            if self._dev_name[0:4] == 'ERR:':
                self._dev_name = "N/A"
                raise ValueError('XY device name is not found in the micromanager device list')
        return self._dev_name

    @staticmethod
    def _ok_check(response, msg):
        """ Checks the response if it was recieved OK."""
        if str(response.decode()) != 'Ok':
            logging.error('{}. Recieved: {}'.format(msg, response))
            return False
        return True

    def startup(self):
        """ Don't move, some stages move automatically but if we can help it we don't want it to move"""
        pass

    def shutdown(self):
        """Don't move, just stay here"""
        pass

    def get_status(self):
        return self.pos

    def read_xy(self):
        xy = self.controller.send_command('xy,get_position\n')
        if type(xy) is not list:
            logging.warning("not in list {}".format(xy))
            return None
        if type(xy[0]) is not float and type(xy[0]) is not int:
            logging.warning("Did not read XY stage position: {}".format(xy))
            return None
        self.pos = self._scale_values(xy)
        return self.pos

    def set_xy(self, xy):
        raw_xy = self._invert_scale(xy)
        rsp = self.controller.send_command(
            'xy,set_position,{},{},{}\n'.format(self._get_xy_device(), raw_xy[0], raw_xy[1]))
        msg = "Did not set XY Stage"
        return self._ok_check(rsp, msg)

    def set_rel_xy(self, rel_xy):
        rel_xy = self._invert_scale(rel_xy)
        rsp = self.controller.send_command(
            'xy,rel_position,{},{},{}\n'.format(self._get_xy_device(), rel_xy[0], rel_xy[1]))
        msg = "Did not set XY stage"
        return self._ok_check(rsp, msg)

    def set_acceleration(self, acceleration):
        """
        Micromanager has no way to set accerlation, so the user must make their best guess
        :return:
        """
        self._acceleration = acceleration

    def set_velocity(self, velocity):
        """
        Micromanager has no way to set the velocity of the stage, so the user must make their best guess
        :param velocity:
        :return:
        """
        self._velocity = velocity

    def get_accleration(self):
        """
        Return the best guess of the XY stage accerlation
        :return:
        """
        return self._accleration

    def get_velocity(self):
        """
        Return the best guess of the XY stage max velocity
        :return: float
        """

        return self._velocity


class PriorXY(XYAbstraction, UtilityControl):

    def __init__(self, controller, role):
        super().__init__(controller, role)

    def startup(self):
        """Prepare the stage for startup"""
        self.read_xy()

    def shutdown(self):
        """Prepare the stage for shutdown"""
        self.read_xy()

    def set_xy(self, xy):
        """ Set the XY position"""
        raw_xy = self._invert_scale(xy)
        response = self.controller.send_command("G {},{}\r".format(raw_xy[0], raw_xy[1]))

    def read_xy(self):
        """ Read the XY position """
        response = self.controller.send_command("P \r".encode()).split(',')
        xy = [eval(x) for x in response]
        xy = self._scale_values(xy)
        self.pos = xy
        return xy

    def set_rel_xy(self, rel_xy):
        """ Moves the stage a relative amount"""
        rel_xy = self._invert_scale(rel_xy)
        rsp = self.controller.send_command("GR {},{}\r".format(rel_xy[0], rel_xy[1]).encode())

    def set_home(self):
        """ Sets the current position as home position for the stage """
        self.controller.send_command("Z \r".encode())
        self.read_xy()

    def go_home(self):
        """ Sends the stage to the home position"""
        self.set_xy([0, 0])

    def set_velocity(self, velocity):
        """
        Sets the max velocity of the stage in mm/s
        :param velocity:
        :return:
        """
        


class XYControlFactory(UtilityFactory):
    """ Determines the type of xy utility object to return according to the daqcontroller id"""

    def build_object(self, controller, role):
        if controller.id == 'micromanager':
            return MicroManagerXY(controller, role)
        elif controller.id == 'prior':
            return PriorXY(controller, role)
        else:
            return None


if __name__ == "__main__":
    from L1.Controllers import MicroManagerController

    ctl = MicroManagerController()
    ctl.open()
    xy = MicroManagerXY(ctl)
    xy.read_xy()
    xy.set_xy([444, 222])
    xy.set_home()
    xy.set_rel_xy([500, 500])
    xy.go_home()