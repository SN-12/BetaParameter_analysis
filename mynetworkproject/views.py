from django.http import HttpResponse
from django.shortcuts import render
from django.template.response import TemplateResponse
from django.contrib.staticfiles.storage import staticfiles_storage
import csv
import os
import xml.etree.ElementTree as ET
from lxml import etree
import io

from mynetworkproject.templatetags.other import run_os_command, run_os_command_rep, run_os_command_propagation

beta = [50, 80, 100, 120, 150, 200, 400, 500, 550, 1000, 1500, 2000, 5000, 20000, 100000]


def index(request):
    # Create a response
    response = TemplateResponse(request, 'index.html', {})

    # Return the response
    return response


def form1(request):
    context = {'x': beta, 'y': [], 'z': []}

    list_src_id = str(request.GET['src_id'])[:-1].split(',')

    y_cnt = 0
    for item in list_src_id:
        y = run_os_command(int(item))
        context['y'].append(y)
        context['z'].append(int(item))
        y_cnt += 1

    print(context)
    # Create a response
    response = TemplateResponse(request, 'form1.html', context=context)

    # Return the response
    return response


def form2(request):
    context = {'x': beta, 'y': [], 'z': []}

    list_src_id = str(request.GET['repetition'])[:-1].split(',')

    y_cnt = 0
    for item in list_src_id:
        y = run_os_command_rep(int(item))
        context['y'].append(y)
        context['z'].append(int(item))
        y_cnt += 1

    print(context)
    # Create a response
    response = TemplateResponse(request, 'form2.html', context=context)

    # Return the response
    return response


def form3(request):
    context = {'x': [], 'y': [], 'z': []}

    list_beta_id = str(request.GET['beta'])[:-1].split(',')

    list_rep_id = str(request.GET['repetition'])[:-1].split(',')

    list_src_id = str(request.GET['src_id'])[:-1].split(',')

    y_cnt = 0
    for item in list_beta_id:
        y = run_os_command_propagation(p_beta_id=int(item), p_source_id=int(list_src_id[0]),
                                       p_repetition_id=int(list_rep_id[0]))

        print(y)
        context['x'].append(y['x'])
        context['y'].append(y['y'])
        context['z'].append(int(item))
        y_cnt += 1

    print(context)
    # Create a response
    response = TemplateResponse(request, 'form3.html', context=context)

    # Return the response
    return response
