from django.template import Library
import xml.etree.ElementTree as ET
import os

register = Library()
beta = [50, 80, 100, 120, 150, 200, 400, 500, 550, 1000, 1500, 2000, 5000, 20000, 100000]


@register.simple_tag
def run_os_command(p_src_id):
    base_path = '/home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/'
    collision_result = []

    xml_file = '/home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/tests/PureFloodingRoutingTest/scenario.xml'
    tree = ET.parse(xml_file)
    root = tree.getroot()

    for z in root.iter('flow'):
        z.set('srcId', str(p_src_id))

    for b in beta:
        for z in root.iter('flow'):
            z.set('beta', str(b))

        tree.write(xml_file)

        # Run Os commands
        collision_file = 'collision.log'

        cmd = base_path + 'bitsimulator -D ' + base_path + 'tests/PureFloodingRoutingTest'
        os.system(cmd)

        cmd = 'grep ^2 ' + base_path + 'tests/PureFloodingRoutingTest/events.log>' + collision_file
        os.system(cmd)

        cmd = 'wc -l ' + collision_file
        collision_result.append(int(os.popen(cmd).read().split(' ')[0]))

        cmd = 'rm ' + collision_file
        os.system(cmd)

    print(beta)
    print(collision_result)

    return collision_result


@register.simple_tag
def run_os_command_rep(p_rep_id):
    base_path = '/home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/'
    collision_result = []

    xml_file = '/home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/tests/PureFloodingRoutingTest/scenario.xml'
    tree = ET.parse(xml_file)
    root = tree.getroot()

    for z in root.iter('flow'):
        z.set('repetitions', str(p_rep_id))

    # tree.write(xml_file)

    for b in beta:
        for z in root.iter('flow'):
            z.set('beta', str(b))

        tree.write(xml_file)

        # Run Os commands
        collision_file = 'collision.log'

        cmd = base_path + 'bitsimulator -D ' + base_path + 'tests/PureFloodingRoutingTest'
        os.system(cmd)

        cmd = 'grep ^2 ' + base_path + 'tests/PureFloodingRoutingTest/events.log>' + collision_file
        os.system(cmd)

        cmd = 'wc -l ' + collision_file
        collision_result.append(int(os.popen(cmd).read().split(' ')[0]))

        cmd = 'rm ' + collision_file
        os.system(cmd)

    print(beta)
    print(collision_result)

    return collision_result


@register.simple_tag
def run_os_command_propagation(p_beta_id, p_repetition_id, p_source_id):
    base_path = '/home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/'
    delay_result = {'x': [], 'y': []}

    xml_file = '/home/snouri/Downloads/mynetworkproject/mynetworkproject/templatetags/bitsimulator-0.9.4+/tests/PureFloodingRoutingTest/scenario.xml'
    tree = ET.parse(xml_file)
    root = tree.getroot()

    for z in root.iter('flow'):
        z.set('repetitions', str(p_repetition_id))

    # tree.write(xml_file)
    for z in root.iter('flow'):
        z.set('srcId', str(p_source_id))

    for z in root.iter('flow'):
        z.set('beta', str(p_beta_id))

    tree.write(xml_file)

    # Run Os commands
    collision_file = 'collision.log'

    cmd = base_path + 'bitsimulator -D ' + base_path + 'tests/PureFloodingRoutingTest'
    os.system(cmd)

    # os.chdir(base_path + 'tests/PureFloodingRoutingTest/')
    cmd = base_path + 'tests/PureFloodingRoutingTest/' + 'firstRec_exe                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              .sh'
    os.system(cmd)

    f = open(base_path + 'tests/PureFloodingRoutingTest/' + 'DelayPerNode.txt', 'r')
    line = 1
    for l in f:
        token_list = str(l).split(' ')
        delay_result['x'].append(int(str(token_list[1]).replace('\n', '')))
        delay_result['y'].append(line)
        line += 1

    return delay_result
