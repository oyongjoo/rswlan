##################################################
#   This script generates and sends specified types of emails.
#   This script is currently can be used only for Gitlab CI automation.
#   If you want to use script separately, make sure that all needed environment
#   variables are exported.
#
#   Script execution parameters:
#   REQUIRED:
#           --type - Type of email. Currently, can be scheduled, release, or failed.
#           --recipients - List of recipients email will be sent to.
#
#   python3 send_email.py --type scheduled --recipient john.doe@renesas.com
#
#   Python version:
#       Script was tested with Python 3.10.6
##################################################

import os
import smtplib
import argparse
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText


def parse_arguments():
    """
    Parses command line arguments to determine the type of email to send and the recipients.

    :return namespace: The parsed command-line arguments, including the email type and recipients.
    """
    parser = argparse.ArgumentParser(description='Send email notifications.')
    parser.add_argument('--type',
                        choices=['scheduled', 'release'],
                        required=True,
                        help='Type of the email to send.')
    parser.add_argument('--recipients', required=True, help='Recipients of the email')
    return parser.parse_args()


def env(env_var):
    """
    Fetches the value of the specified environment variable.

    :param str env_var: The name of the environment variable to retrieve.
    :return str: The value of the environment variable, or empty string if not set.
    """
    env_var = os.environ.get(env_var)
    if not env_var:
        return ""
    return env_var


def create_scheduled_email_body():
    """
    Constructs the HTML body for the scheduled scan report email.

    This function uses environment variables to fill in the details
    specific to the scheduled scan report, including project URLs,
    commit information, and various report links.

    :return str: The HTML content for the scheduled scan report email.
    """

    return f"""
    <html>
      <body>
        <h2>[{env('CI_PROJECT_NAME').upper()}] [{env('CI_COMMIT_REF_NAME')}] {env('SCHEDULE')} scan report - {env('DATE')}</h2> <br>
        <b>Description:</b> &emsp;Scheduled scan report for the {env('SCHEDULE')} build at {env('DATE')} <br>
        <b>Sonar Report:</b>&emsp;<a href="{env('SONAR_SCAN_REPORT_URL')}">link here</a> <br>
        <b>BlackDuck Report:</b>&emsp;<a href="{env('BLACKDUCK_SCAN_REPORT_URL')}">link here</a> <br>
        <b>Repository URL:</b>&emsp;<a href="{env('CI_PROJECT_URL')}">link here</a> <br>
        <b>Commit Hash:</b> &emsp;{env('CI_COMMIT_SHA')} <br>
        <b>Build info:</b>&emsp;<a href="{env('CI_PIPELINE_URL')}">link here</a> <br>
      </body>
    </html>
    """


def create_release_email_body():
    """
    Constructs the HTML body for the release notification email.

    This function constructs an email body tailored for release notifications,
    utilizing various environment variables to provide context such as release notes,
    binary locations, version details, and submodule information.

    :return str: The HTML content for the release notification email.
    """
    return f"""
    <html>
      <body>
        <h2>[RELEASE] [{env('CI_PROJECT_NAME').upper()}] [{env('CI_COMMIT_TAG')}]</h2> <br>
        <p><b>Description:</b>&emsp;Driver release for RSBT </p>
        <p><b>Release notes:</b>&emsp;<a href="{env('RELEASE_NOTES_URL')}">{env('RELEASE_NOTES_FILE')}</a></p>
        <p><b>Driver location:</b>&emsp;<a href="{env('UPLOAD_ARTIFACTORY_URL')}">link here</a></p>
        <p><b>RF tool location:</b>&emsp;<a href="{env('ARTIFACTORY_RF_TOOL_URL')}">link here</a></p>
        <p><b>Repository URL:</b>&emsp;<a href="{env('CI_PROJECT_URL')}">link here</a></p>
        <p><b>Release Version:</b>&emsp;{env('CI_COMMIT_TAG')} </p>
        <p><b>Commit Hash:</b>&emsp;{env('CI_COMMIT_SHA')} </p>
        <p><b>Build job info:</b> <a href="{env('CI_JOB_URL')}">link here</a></p>
        <p><b>Tag Author:</b> {env('GIT_TAG_AUTHOR')}</p>
      </body>
    </html>
    """


def send_email(email_body, email_subject, recipients):
    """
    Sends an email using the constructed HTML body and subject to the specified recipients
    by connecting to the SMTP server.

    :param str email_body (str): The HTML content of the email.
    :param str email_subject (str): The subject of the email to be sent.
    :param str recipients (str): A comma-separated string of email recipients.
    """

    sender_email = env("RENESAS_M365_SMTP_USERNAME") or 'automation@dm.renesas.com'
    sender_password = env("RENESAS_M365_SMTP_PASSWORD") or ''
    sender_host = env("RENESAS_M365_SMTP_HOST") or 'localhost'
    sender_port = env("RENESAS_M365_SMTP_PORT") or '25'

    # Create the email message
    msg = MIMEMultipart()
    msg['From'] = sender_email
    msg['To'] = ", ".join(recipients.split(" "))
    msg['Subject'] = email_subject

    msg.attach(MIMEText(email_body, 'html'))

    # Send the email using SMTP
    try:
        with smtplib.SMTP(sender_host, sender_port) as server:
            server.set_debuglevel(1)
            server.ehlo()
            if int(sender_port) != 25:
                server.starttls()
                server.login(sender_email, sender_password)
            print("Sending email...")
            print(f"SMTP server: {sender_host}:{sender_port}")
            print(f"Sender(From): {sender_email}")
            print(f"Subject: {email_subject}")
            print(f"Recipients: {recipients}")
            server.send_message(msg)
            print("Email sent successfully!")
    except Exception as e:
        print(f"Error occurred while sending email: {e}")


if __name__ == "__main__":
    args = parse_arguments()

    if args.type == 'scheduled':
        print("Scheduled email will be sent.")
        email_body = create_scheduled_email_body()
        email_subject = f"[{env('CI_PROJECT_NAME').upper()}] [{env('CI_COMMIT_REF_NAME')}] {env('SCHEDULE')} scan report"
    elif args.type == 'release':
        print("Release email will be sent.")
        email_body = create_release_email_body()
        email_subject = f"[RELEASE] [{env('CI_PROJECT_NAME').upper()}] [{env('CI_COMMIT_TAG')}]"

    send_email(email_body, email_subject, args.recipients)