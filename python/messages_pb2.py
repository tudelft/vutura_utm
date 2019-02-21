# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: messages.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf.internal import enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='messages.proto',
  package='',
  syntax='proto2',
  serialized_pb=_b('\n\x0emessages.proto\"[\n\nGPSMessage\x12\x11\n\ttimestamp\x18\x01 \x02(\x04\x12\x0b\n\x03lat\x18\x02 \x02(\x11\x12\x0b\n\x03lon\x18\x03 \x02(\x11\x12\x0f\n\x07\x61lt_msl\x18\x04 \x02(\x11\x12\x0f\n\x07\x61lt_agl\x18\x05 \x02(\x11\"\x1d\n\x0cUavHeartbeat\x12\r\n\x05\x61rmed\x18\x01 \x01(\x08\"+\n\nMavCommand\x12\x1d\n\x07\x63ommand\x18\x01 \x02(\x0e\x32\x0c.MavCommands\"g\n\nFlightplan\x12&\n\x08waypoint\x18\x01 \x03(\x0b\x32\x14.Flightplan.Waypoint\x1a\x31\n\x08Waypoint\x12\x0b\n\x03lat\x18\x01 \x02(\x11\x12\x0b\n\x03lon\x18\x02 \x02(\x11\x12\x0b\n\x03\x61lt\x18\x03 \x01(\x11*I\n\x0bMavCommands\x12\x11\n\rSTART_MISSION\x10\x01\x12\x11\n\rPAUSE_MISSION\x10\x02\x12\x14\n\x10\x43ONTINUE_MISSION\x10\x03')
)
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

_MAVCOMMANDS = _descriptor.EnumDescriptor(
  name='MavCommands',
  full_name='MavCommands',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='START_MISSION', index=0, number=1,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='PAUSE_MISSION', index=1, number=2,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='CONTINUE_MISSION', index=2, number=3,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=292,
  serialized_end=365,
)
_sym_db.RegisterEnumDescriptor(_MAVCOMMANDS)

MavCommands = enum_type_wrapper.EnumTypeWrapper(_MAVCOMMANDS)
START_MISSION = 1
PAUSE_MISSION = 2
CONTINUE_MISSION = 3



_GPSMESSAGE = _descriptor.Descriptor(
  name='GPSMessage',
  full_name='GPSMessage',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='timestamp', full_name='GPSMessage.timestamp', index=0,
      number=1, type=4, cpp_type=4, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='lat', full_name='GPSMessage.lat', index=1,
      number=2, type=17, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='lon', full_name='GPSMessage.lon', index=2,
      number=3, type=17, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='alt_msl', full_name='GPSMessage.alt_msl', index=3,
      number=4, type=17, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='alt_agl', full_name='GPSMessage.alt_agl', index=4,
      number=5, type=17, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=18,
  serialized_end=109,
)


_UAVHEARTBEAT = _descriptor.Descriptor(
  name='UavHeartbeat',
  full_name='UavHeartbeat',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='armed', full_name='UavHeartbeat.armed', index=0,
      number=1, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=111,
  serialized_end=140,
)


_MAVCOMMAND = _descriptor.Descriptor(
  name='MavCommand',
  full_name='MavCommand',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='command', full_name='MavCommand.command', index=0,
      number=1, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=142,
  serialized_end=185,
)


_FLIGHTPLAN_WAYPOINT = _descriptor.Descriptor(
  name='Waypoint',
  full_name='Flightplan.Waypoint',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='lat', full_name='Flightplan.Waypoint.lat', index=0,
      number=1, type=17, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='lon', full_name='Flightplan.Waypoint.lon', index=1,
      number=2, type=17, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='alt', full_name='Flightplan.Waypoint.alt', index=2,
      number=3, type=17, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=241,
  serialized_end=290,
)

_FLIGHTPLAN = _descriptor.Descriptor(
  name='Flightplan',
  full_name='Flightplan',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='waypoint', full_name='Flightplan.waypoint', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[_FLIGHTPLAN_WAYPOINT, ],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=187,
  serialized_end=290,
)

_MAVCOMMAND.fields_by_name['command'].enum_type = _MAVCOMMANDS
_FLIGHTPLAN_WAYPOINT.containing_type = _FLIGHTPLAN
_FLIGHTPLAN.fields_by_name['waypoint'].message_type = _FLIGHTPLAN_WAYPOINT
DESCRIPTOR.message_types_by_name['GPSMessage'] = _GPSMESSAGE
DESCRIPTOR.message_types_by_name['UavHeartbeat'] = _UAVHEARTBEAT
DESCRIPTOR.message_types_by_name['MavCommand'] = _MAVCOMMAND
DESCRIPTOR.message_types_by_name['Flightplan'] = _FLIGHTPLAN
DESCRIPTOR.enum_types_by_name['MavCommands'] = _MAVCOMMANDS

GPSMessage = _reflection.GeneratedProtocolMessageType('GPSMessage', (_message.Message,), dict(
  DESCRIPTOR = _GPSMESSAGE,
  __module__ = 'messages_pb2'
  # @@protoc_insertion_point(class_scope:GPSMessage)
  ))
_sym_db.RegisterMessage(GPSMessage)

UavHeartbeat = _reflection.GeneratedProtocolMessageType('UavHeartbeat', (_message.Message,), dict(
  DESCRIPTOR = _UAVHEARTBEAT,
  __module__ = 'messages_pb2'
  # @@protoc_insertion_point(class_scope:UavHeartbeat)
  ))
_sym_db.RegisterMessage(UavHeartbeat)

MavCommand = _reflection.GeneratedProtocolMessageType('MavCommand', (_message.Message,), dict(
  DESCRIPTOR = _MAVCOMMAND,
  __module__ = 'messages_pb2'
  # @@protoc_insertion_point(class_scope:MavCommand)
  ))
_sym_db.RegisterMessage(MavCommand)

Flightplan = _reflection.GeneratedProtocolMessageType('Flightplan', (_message.Message,), dict(

  Waypoint = _reflection.GeneratedProtocolMessageType('Waypoint', (_message.Message,), dict(
    DESCRIPTOR = _FLIGHTPLAN_WAYPOINT,
    __module__ = 'messages_pb2'
    # @@protoc_insertion_point(class_scope:Flightplan.Waypoint)
    ))
  ,
  DESCRIPTOR = _FLIGHTPLAN,
  __module__ = 'messages_pb2'
  # @@protoc_insertion_point(class_scope:Flightplan)
  ))
_sym_db.RegisterMessage(Flightplan)
_sym_db.RegisterMessage(Flightplan.Waypoint)


# @@protoc_insertion_point(module_scope)
