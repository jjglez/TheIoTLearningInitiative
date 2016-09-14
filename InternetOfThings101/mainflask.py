#!/usr/bin/python

from flask import Flask
from flask_restful import Api, Resource

app = Flask(__name__)
api = Api(app)

class DataSensorRestApi(Resource):
    def get(self):
        data = 'This is data from a sensor'
        return data

api.add_resource(DataSensorRestApi, '/datasensor')

if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)
